#include <iostream>
#include <string>
#include <vector>
#include <memory>
#include <mutex>
#include <thread>
#include <chrono>
#include <sstream>
#include "SQLiteCpp/SQLiteCpp.h"
#include "spdlog/spdlog.h"
#include "asio.hpp"
#include "cereal/archives/binary.hpp"
#include <cereal/types/string.hpp>
#include "server/server_logging.hpp"
#include "server/server_network.hpp"
#include "server/game_manager.hpp"

#include "game_state.hpp"
//#include "cereal/cereal.hpp"

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif

// Server constants
constexpr int PORT = 12345;

struct ServerData {
    GameState gameState;
    std::vector<std::shared_ptr<asio::ip::tcp::socket>> clients;
    std::mutex stateMutex;
    std::mutex clientsMutex;
    std::shared_ptr<asio::io_context> io_context; // Add the io_context here

    ServerData() : io_context(std::make_shared<asio::io_context>()) {} // Initialize io_context
};


// Launch a terminal for each player
void launchTerminal(int playerIndex) {
    std::string command;
#ifdef _WIN32
    // Windows: Use 'start' to open a new command prompt
    command = "start cmd /k \"canasta_client.exe " + std::to_string(playerIndex) + "\"";
#elif __APPLE__
    // macOS: Use 'osascript' to run a command in a new Terminal window
    command = "osascript -e 'tell application \"Terminal\" to do script \"cd '$PWD' && ./canasta_client " + std::to_string(playerIndex) + "\"'";
#else
    // Linux: Use 'gnome-terminal' or 'xterm' to open a new terminal window
    command = "gnome-terminal -- bash -c './canasta_client " + std::to_string(playerIndex) + "; exec bash'";
#endif
    spdlog::info("Launching terminal for Player {}", playerIndex + 1);
    system(command.c_str());
}

void detectOSAndLaunchTerminals(int numPlayers) {
    for (int i = 0; i < numPlayers; ++i) {
        std::this_thread::sleep_for(std::chrono::milliseconds(500)); // Add delay to ensure terminals launch correctly
        launchTerminal(i);
    }
}

// Broadcast the game state to all connected clients
void broadcastGameState(ServerData& serverData) {
    std::ostringstream os;
    {
        cereal::BinaryOutputArchive archive(os);
        archive(serverData.gameState);
    }
    std::string serializedData = os.str() + "\n";

    std::lock_guard<std::mutex> lock(serverData.clientsMutex);
    for (const auto& client : serverData.clients) {
        asio::write(*client, asio::buffer(serializedData));
    }
}

// Handle a single client's connection
void handleClient(std::shared_ptr<asio::ip::tcp::socket> socket, ServerData& serverData) {
    try {
        spdlog::info("Client connected: {}", socket->remote_endpoint().address().to_string());
        while (true) {
            asio::streambuf buf;
            asio::read_until(*socket, buf, '\n');
            std::istream is(&buf);
            GameState updatedState;
            {
                cereal::BinaryInputArchive archive(is);
                archive(updatedState);
            }

            {
                std::lock_guard<std::mutex> lock(serverData.stateMutex);
                serverData.gameState = updatedState;
            }

            spdlog::info("Game state updated by Player {}: {}", updatedState.currentPlayer, updatedState.publicMessage);
            broadcastGameState(serverData);
        }
    } catch (const std::exception& e) {
        spdlog::error("Client disconnected: {}", e.what());
        std::lock_guard<std::mutex> lock(serverData.clientsMutex);
        serverData.clients.erase(std::remove(serverData.clients.begin(), serverData.clients.end(), socket), serverData.clients.end());
    }
}

// Start the server and propagate the initial game state
void startServer(ServerData& serverData, int numPlayers) {
    asio::ip::tcp::acceptor acceptor(*serverData.io_context, asio::ip::tcp::endpoint(asio::ip::tcp::v4(), PORT));


    spdlog::info("Server started on port {}", PORT);

    for (int i = 0; i < numPlayers; ++i) {
        auto socket = std::make_shared<asio::ip::tcp::socket>(*serverData.io_context);
        acceptor.accept(*socket);

        spdlog::info("Player {} connected.", i + 1);

        // Add the player to the clients vector
        {
            std::lock_guard<std::mutex> lock(serverData.clientsMutex);
            serverData.clients.push_back(socket);
        }

        // Propagate the initial game state to the connected client
        spdlog::info("Sending initial game state to Player {}", i + 1);
        std::ostringstream os;
        {
            cereal::BinaryOutputArchive archive(os);
            archive(serverData.gameState);
        }
        std::string serializedData = os.str() + "\n";
        asio::write(*socket, asio::buffer(serializedData));

        // Launch a thread to handle the client
        std::thread(handleClient, socket, std::ref(serverData)).detach();
    }
    // Do not block here, but keep io_context running in a separate thread
    std::thread([io_context = serverData.io_context] {
        io_context->run(); // Keep io_context alive for handling async operations
    }).detach();
}

void oldMain() {
    ServerData serverData;
    serverData.gameState = GameState{0, "Welcome to the game!"};

    int playersCount = 4;
    spdlog::info("Starting the network server...");
    startServer(serverData, playersCount);

    // Keep the main thread alive
    while (true) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}


int main() {
    initLogger();
    spdlog::info("----------Canasta Server is starting----------");
    int playersCount = 4;

    try {
        // 3) construct the core game manager
        GameManager gameManager(static_cast<std::size_t>(playersCount));

        // 4) set up ASIO
        asio::io_context ioContext;

        // listen on all interfaces, port SERVER_PORT
        asio::ip::tcp::endpoint endpoint{ asio::ip::tcp::v4(), PORT};
        ServerNetwork server(ioContext, endpoint, gameManager);

        // 5) begin accepting connections
        server.startAccept();

        // 6) fire up each client in its own terminal window
        spdlog::info("Launching {} player terminals…", playersCount);
        for (int i = 0; i < playersCount; ++i) {
            launchTerminal(i);
            // slight stagger so they don’t all hammer the server simultaneously
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
        }

        // 7) run the ASIO loop (this will block until you call ioContext.stop())
        ioContext.run();

        spdlog::info("Server shutting down cleanly.");
    }
    catch (std::exception& e) {
        spdlog::error("Unhandled exception: {}", e.what());
        return 1;
    }
    spdlog::shutdown();
    return 0;
}
