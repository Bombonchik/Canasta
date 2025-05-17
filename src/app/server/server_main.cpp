#include <iostream>
#include <string>
#include <vector>
#include <memory>
#include <mutex>
#include <thread>
#include <chrono>
#include <sstream>
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
    // Linux: Use 'gnome-terminal' to open a new terminal window
    command = "gnome-terminal -- bash -c './canasta_client " + std::to_string(playerIndex) + "; exec bash'";
#endif
    spdlog::info("Launching terminal for Player {}", playerIndex + 1);
    system(command.c_str());
}

void detectOSAndLaunchTerminals(int playersCount) {
    spdlog::info("Launching {} player terminals…", playersCount);
    for (int i = 0; i < playersCount; ++i) {
        launchTerminal(i);
        // slight stagger so they don’t all hammer the server simultaneously
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

int main(int argc, char* argv[]) {
    initLogger();
    if (argc < 2) {
        spdlog::error("Number of players not specified!");
        return 1;
    }
    int playersCount = std::stoi(argv[1]);
    if (playersCount != 2 && playersCount != 4) {
        spdlog::error("Invalid number of players. Must be either 2 or 4.");
        return 1;
    }
    spdlog::info("----------Canasta Server is starting----------");

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
        detectOSAndLaunchTerminals(playersCount);

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
