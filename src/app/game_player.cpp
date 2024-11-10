#include <iostream>
#include <string>
#include <asio.hpp>
#include <sstream>
#include "game_state.hpp"
#include "spdlog/spdlog.h"
#include "spdlog/sinks/stdout_color_sinks.h"
#include "cereal/archives/binary.hpp"
#include <cereal/types/string.hpp>

// Constants
constexpr int SERVER_PORT = 12345;
constexpr int REFRESH_DELAY_MS = 100; // Refresh console every 500 ms for updates

void clearConsole() {
#ifdef _WIN32
    system("cls");
#else
    system("clear");
#endif
}

// Function to configure spdlog for clean output
void configureLogger() {
    auto console = spdlog::stdout_color_mt("game_logger");
    spdlog::set_default_logger(console);
    spdlog::set_pattern("%v"); // Only display the message (no timestamp, level, etc.)
}

// Function to receive the game state from the server
GameState receiveGameState(asio::ip::tcp::socket& socket) {
    asio::streambuf buf;
    asio::read_until(socket, buf, '\n');
    std::istream is(&buf);
    GameState gameState;
    cereal::BinaryInputArchive archive(is);
    archive(gameState);
    return gameState;
}

// Function to send the player's input to the server
void sendPlayerInput(asio::ip::tcp::socket& socket, const GameState& gameState) {
    std::ostringstream os;
    {
        cereal::BinaryOutputArchive archive(os);
        archive(gameState);
    }
    std::string serializedData = os.str() + "\n";
    asio::write(socket, asio::buffer(serializedData));
}

// Function to display the game state
void displayGameState(const GameState& gameState, int playerIndex) {
    clearConsole(); // Clear the console before displaying updated information
    spdlog::info("Public Message: {}", gameState.publicMessage);
    spdlog::info("Player {}, {}", playerIndex + 1,
                 gameState.currentPlayer == playerIndex ? "it's your turn." : "it's not your turn.");
    if (gameState.currentPlayer != playerIndex) {
        spdlog::info("Your private message: Hello from Player {}", playerIndex + 1);
    }
}

void playGame(asio::ip::tcp::socket& socket, int playerIndex) {
    while (true) {
        try {
            GameState gameState = receiveGameState(socket); // Get the latest game state
            displayGameState(gameState, playerIndex); // Display the game state

            if (gameState.currentPlayer == playerIndex) {
                std::string newMessage;
                spdlog::info("Enter a new public message: ");
                std::getline(std::cin, newMessage);

                gameState.publicMessage = newMessage;
                gameState.currentPlayer = (gameState.currentPlayer + 1) % 4; // Pass turn to the next player
                sendPlayerInput(socket, gameState); // Send the updated game state to the server
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(REFRESH_DELAY_MS));
        } catch (const std::exception& e) {
            spdlog::error("Connection lost: {}", e.what());
            break;
        }
    }
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        spdlog::error("Player index not specified!");
        return 1;
    }

    int playerIndex = std::stoi(argv[1]);

    try {
        configureLogger(); // Configure the logger

        asio::io_context io_context;
        asio::ip::tcp::socket socket(io_context);
        std::this_thread::sleep_for(std::chrono::milliseconds(600));

        // Connect to the server
        socket.connect(asio::ip::tcp::endpoint(asio::ip::address::from_string("127.0.0.1"), SERVER_PORT));
        spdlog::info("Connected to the server at 127.0.0.1:{}", SERVER_PORT);

        playGame(socket, playerIndex); // Start the game loop
    } catch (const std::exception& e) {
        spdlog::error("Error: {}", e.what());
        return 1;
    }

    return 0;
}
