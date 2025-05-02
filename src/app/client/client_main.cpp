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
#include "client/canasta_console.hpp"

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

void displayClientGameState(const ClientGameState& gameState, bool isMyTurn) {
    //clearConsole(); // Clear the console before displaying updated information
    spdlog::info("Is my turn: {}", isMyTurn);
    spdlog::info("My player {}", gameState.myPlayerData.getName());
    if (isMyTurn) {
        const auto& handCards = gameState.myPlayerData.getHand().getCards();
        for (std::size_t i = 0; i < handCards.size(); ++i) {
            spdlog::info("Card {}: {}{}{}", i + 1, fg_color(handCards[i].getColor()), handCards[i].toString(), "\033[0m");
        }
    } 
}

void testCanastaConsole() {
    CanastaConsole ui;
    ui.clear();
    for (int i = 0; i < 20; ++i) {
        ui.print("Testing CanastaConsole: " + std::to_string(i), Color::BrightCyan);
    }
    ui.clear();

    ui.print("CanastaConsole Test Harness", Color::BrightMagenta);
    ui.print("--------------------------------", Color::BrightWhite);

    // Demonstrate colors
    ui.print("Default color text");
    ui.print("Red message", Color::Red);
    ui.print("Green message", Color::Green);
    ui.print("Yellow message", Color::Yellow);
    ui.print("Blue message", Color::Blue);
    ui.print("Magenta message", Color::Magenta);
    ui.print("Cyan message", Color::Cyan);
    ui.print("White message", Color::White);

    ui.print("BrightRed message", Color::BrightRed);
    ui.print("BrightGreen message", Color::BrightGreen);
    ui.print("BrightYellow message", Color::BrightYellow);
    ui.print("BrightBlue message", Color::BrightBlue);
    ui.print("BrightMagenta message", Color::BrightMagenta);
    ui.print("BrightCyan message", Color::BrightCyan);
    ui.print("BrightWhite message", Color::BrightWhite);

    // Demonstrate UTF-8
    ui.print("UTF-8 symbols: ✔ ✓ → ← ▲ ▼ ★ ☆ ☯ ☢", Color::BrightCyan);
    ui.print("Unicode emojis: 😀 🎉 🚀 🃏", Color::Yellow);
    ui.print("CJK characters: 漢字 仮名 가나다", Color::Green);

    // Demonstrate printing without newline
    ui.print("Loading: [", Color::BrightBlue, false);
    for (int i = 0; i <= 10; ++i) {
        ui.print(std::to_string(i * 10) + "%", Color::BrightGreen, false);
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        ui.print("Loading: [" + std::to_string((i+1)*10) + "%] ", Color::BrightBlue, false);
    }
    ui.print("] Done!", Color::BrightGreen);

    // Pause before exit
    ui.print("Press Enter to exit...", Color::BrightWhite, false);
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
        //CanastaConsole console;
        //console.print("Starting Canasta Client for " + playerName, Color::Green);
        testCanastaConsole();


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
            std::this_thread::sleep_for(std::chrono::milliseconds(40000));
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
