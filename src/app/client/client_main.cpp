#include <iostream>
#include <string>
#include <asio.hpp>
#include <sstream>
#include "game_state.hpp"
#include "spdlog/spdlog.h"
#include "spdlog/sinks/stdout_color_sinks.h"
#include "cereal/archives/binary.hpp"
#include <cereal/types/string.hpp>
#include "client/client_network.hpp"

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

static std::string fg_color(CardColor c) {
    switch(c) {
        case CardColor::RED:   return "\033[31m";  // ANSI “set foreground to red”
        //case CardColor::BLACK: return "\033[90m";  // ANSI “bright-black” (gray)
        case CardColor::BLACK: return "\033[37m";  // ANSI “bright-black” (gray)
    }
    return "\033[0m";
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

void displayClientGameState(const ClientGameState& gameState, bool isMyTurn) {
    //clearConsole(); // Clear the console before displaying updated information
    spdlog::info("Is my turn: {}", isMyTurn);
    spdlog::info("My player {}", gameState.myPlayerData.getName());
    if (isMyTurn) {
        const auto& handCards = gameState.myPlayerData.getHand().getCards();
        for (std::size_t i = 0; i < handCards.size(); ++i) {
            spdlog::info("Card {}: {}{}{}", i + 1, fg_color(handCards[i].getColor()), handCards[i].toString(), "\033[0m");
        }
        //spdlog::info("{}{}{}", fg_color(card.getColor()), card.toString(), "\033[0m");
    } 
}


int main(int argc, char* argv[]) {
    if (argc < 2) {
        spdlog::error("Player index not specified!");
        return 1;
    }

    int playerIndex = std::stoi(argv[1]);
    std::string playerName = "Player" + std::to_string(playerIndex + 1);

    try {
        configureLogger(); // Configure the logger

        asio::io_context ioContext;
        std::this_thread::sleep_for(std::chrono::milliseconds(500 * playerIndex));

        // Connect to the server
        auto clientNetwork = std::make_shared<ClientNetwork>(ioContext);
        //clientNetwork->connect("127.0.0.1", std::to_string(SERVER_PORT), playerName);
        if (clientNetwork->isConnected()) {
            spdlog::info("Connected to the server at 127.0.0.1");
        }
        clientNetwork->setOnLoginSuccess([playerName]() {
            spdlog::info("[{}] Login succeeded.", playerName);
        });
        clientNetwork->setOnLoginFailure([&](const std::string& reason) {
            spdlog::error("[{}] Login failed: {}", playerName, reason);
            ioContext.stop();
        });
        clientNetwork->setOnDisconnect([&]() {
            spdlog::info("[{}] Disconnected from server.", playerName);
            ioContext.stop();
        });
        clientNetwork->setOnActionError([&](const ActionError& error) {
            if (error.status.has_value()) {
                spdlog::info("[{}] Action status {}: {}", playerName, static_cast<int>(error.status.value()), error.message);
            } else {
                spdlog::info("[{}] Action status: {}", playerName, error.message);
            }
        });
        bool drawDeck = false;
        bool discarded = false;
        clientNetwork->setOnGameStateUpdate([&](const ClientGameState& gameState) {
            spdlog::info("[{}] Received game state update.", playerName);
            // Handle game state update
            // For example, display the game state or process it further
            PlayerPublicInfo myInfo;
            for(const auto& playerInfo : gameState.allPlayersPublicInfo) {
                if (playerInfo.name == playerName) {
                    myInfo = playerInfo;
                    break;
                }
            }
            displayClientGameState(gameState, myInfo.isCurrentTurn);
            std::this_thread::sleep_for(std::chrono::milliseconds(2000));
            if (myInfo.isCurrentTurn || rand() * (playerIndex + 1) % 19 == 0) {
                if (!drawDeck) {
                    clientNetwork->sendDrawDeck();
                    drawDeck = true;
                }
                if (!discarded) {
                    clientNetwork->sendDiscard(gameState.myPlayerData.getHand().getCards()[3]);
                    discarded = true;
                }
                
            } else {
                spdlog::info("[{}] Waiting for your turn...", playerName);
                drawDeck = false;
                discarded = false;
            }
        });

        spdlog::info("[{}] Attempting to connect to 127.0.0.1:{}", playerName, SERVER_PORT);
        clientNetwork->connect("127.0.0.1", std::to_string(SERVER_PORT), playerName);

        std::this_thread::sleep_for(std::chrono::milliseconds(50 * playerIndex));

        ioContext.run();
    } catch (const std::exception& e) {
        spdlog::error("Error: {}", e.what());
        return 1;
    }

    return 0;

}
