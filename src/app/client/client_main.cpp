#include <iostream>
#include <string>
#include <asio.hpp>
#include <sstream>
#include <algorithm>
#include "game_state.hpp"
#include "spdlog/spdlog.h"
#include "spdlog/sinks/stdout_color_sinks.h"
#include "cereal/archives/binary.hpp"
#include <cereal/types/string.hpp>
#include "client/client_network.hpp"
#include "client/canasta_console.hpp"
#include "client/game_view.hpp"
#include "client/client_controller.hpp"

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

std::vector<MeldView> getMeldViewsFromTeamRoundState(const TeamRoundState& teamRoundState) {
    std::vector<MeldView> meldViews;
    auto redThreeMeld = teamRoundState.getRedThreeMeld();
    if (redThreeMeld->isInitialized())
        meldViews.push_back({Rank::Three, redThreeMeld->getCards()});
    auto blackThreeMeld = teamRoundState.getBlackThreeMeld();
    if (blackThreeMeld->isInitialized())
        meldViews.push_back({Rank::Three, blackThreeMeld->getCards()});
    for (int r = static_cast<int>(Rank::Four); r <= static_cast<int>(Rank::Ace); ++r) {
        auto rank = static_cast<Rank>(r);
        auto meld = teamRoundState.getMeldForRank(rank);
        if (meld->isInitialized())
            meldViews.push_back({rank, meld->getCards()});
    }
    return meldViews;
}

BoardState getBoardState(const ClientGameState& gameState) {
    BoardState boardState;
    boardState.myTeamMelds = getMeldViewsFromTeamRoundState(gameState.myTeamState); // wrong temporary
    boardState.opponentTeamMelds = getMeldViewsFromTeamRoundState(gameState.opponentTeamState);
    boardState.myHand = gameState.myPlayerData.getHand();
    boardState.deckState = gameState.deckState;
    boardState.myPlayer = gameState.allPlayersPublicInfo[0]; // Assuming the first player is the current player
    if (gameState.allPlayersPublicInfo.size() == 2) {
        boardState.oppositePlayer = gameState.allPlayersPublicInfo[1]; // Assuming the second player is the opponent
    } else if (gameState.allPlayersPublicInfo.size() == 4) {
        boardState.leftPlayer = gameState.allPlayersPublicInfo[1]; // Assuming the second player is the left player
        boardState.oppositePlayer = gameState.allPlayersPublicInfo[2]; // Assuming the third player is the opponent
        boardState.rightPlayer = gameState.allPlayersPublicInfo[3]; // Assuming the fourth player is the right player
    }
    boardState.myTeamTotalScore = gameState.myTeamTotalScore;
    boardState.opponentTeamTotalScore = gameState.opponentTeamTotalScore;
    boardState.myTeamMeldPoints = gameState.myTeamState.calculateMeldPoints();
    boardState.opponentTeamMeldPoints = gameState.opponentTeamState.calculateMeldPoints();
    return boardState;
}

void printPlayerPublicInfos(const std::vector<PlayerPublicInfo>& playerInfos) {
    for (const auto& playerInfo : playerInfos) {
        std::string color = playerInfo.isCurrentTurn ? "\033[32m" : "\033[0m"; // Green for current turn
        spdlog::info("{}{} ({} cards){}", color, playerInfo.name, playerInfo.handCardCount, "\033[0m");
    }
}

void testCanastaConsole() {
    CanastaConsole ui;
    ui.clear();
    for (int i = 0; i < 20; ++i) {
        ui.print("Testing CanastaConsole: " + std::to_string(i), CanastaConsole::Color::BrightCyan);
    }
    ui.clear();

    ui.print("CanastaConsole Test Harness", CanastaConsole::Color::BrightMagenta);
    ui.print("--------------------------------", CanastaConsole::Color::BrightWhite);

    // Demonstrate colors
    ui.print("Default color text");
    ui.print("Red message", CanastaConsole::Color::Red);
    ui.print("Green message", CanastaConsole::Color::Green);
    ui.print("Yellow message", CanastaConsole::Color::Yellow);
    ui.print("Blue message", CanastaConsole::Color::Blue);
    ui.print("Magenta message", CanastaConsole::Color::Magenta);
    ui.print("Cyan message", CanastaConsole::Color::Cyan);
    ui.print("White message", CanastaConsole::Color::White);

    ui.print("BrightRed message", CanastaConsole::Color::BrightRed);
    ui.print("BrightGreen message", CanastaConsole::Color::BrightGreen);
    ui.print("BrightYellow message", CanastaConsole::Color::BrightYellow);
    ui.print("BrightBlue message", CanastaConsole::Color::BrightBlue);
    ui.print("BrightMagenta message", CanastaConsole::Color::BrightMagenta);
    ui.print("BrightCyan message", CanastaConsole::Color::BrightCyan);
    ui.print("BrightWhite message", CanastaConsole::Color::BrightWhite);

    // Demonstrate UTF-8
    ui.print("UTF-8 symbols: ✔ ✓ → ← ▲ ▼ ★ ☆ ☯ ☢", CanastaConsole::Color::BrightCyan);
    ui.print("Unicode emojis: 😀 🎉 🚀 🃏", CanastaConsole::Color::Yellow);
    ui.print("CJK characters: 漢字 仮名 가나다", CanastaConsole::Color::Green);

    // Demonstrate printing without newline
    ui.print("Loading: [", CanastaConsole::Color::BrightBlue, false);
    for (int i = 0; i <= 10; ++i) {
        ui.print(std::to_string(i * 10) + "%", CanastaConsole::Color::BrightGreen, false);
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        ui.print("Loading: [" + std::to_string((i+1)*10) + "%] ", CanastaConsole::Color::BrightBlue, false);
    }
    ui.print("] Done!", CanastaConsole::Color::BrightGreen);

    // Pause before exit
    ui.print("Press Enter to exit...", CanastaConsole::Color::BrightWhite, false);
}

void testShowMessagesWithBoard(GameView& gameView, BoardState& boardState, std::vector<std::string> messages) {
    std::vector<std::string> myMessages;
    if (messages.empty()) {
        myMessages = {
            "This is a test message 1",
            "This is a test message 2",
        };
    } else {
        myMessages = messages;
    }
    gameView.showStaticBoardWithMessages(myMessages, boardState);
}

int oldMain(int argc, char* argv[]) {
    if (argc < 2) {
        spdlog::error("Player index not specified!");
        return 1;
    }

    int playerIndex = std::stoi(argv[1]);
    std::string playerName = "Player" + std::to_string(playerIndex + 1);
    GameView gameView;
    CanastaConsole console;

    try {
        configureLogger(); // Configure the logger
        //CanastaConsole console;
        //console.print("Starting Canasta Client for " + playerName, Color::Green);
        //testCanastaConsole();


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
            //spdlog::info("[{}] Received game state update.", playerName);
            // Handle game state update

            //printPlayerPublicInfos(gameState.allPlayersPublicInfo);
            bool isMyTurn = gameState.allPlayersPublicInfo[0].isCurrentTurn;

            //displayClientGameState(gameState, isMyTurn);
            auto boardState = getBoardState(gameState);
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
            gameView.restoreInput();
            testShowMessagesWithBoard(gameView, boardState, {gameState.lastActionDescription});
            std::this_thread::sleep_for(std::chrono::milliseconds(rand()%1000));
            //if (myInfo.isCurrentTurn || rand() * (playerIndex + 1) % 19 == 0) {
            if (isMyTurn) {
                if (!drawDeck) {
                    clientNetwork->sendDrawDeck();
                    drawDeck = true;
                }
                if (!discarded) {
                    clientNetwork->sendDiscard(gameState.myPlayerData.getHand().getCards()[rand() % std::min<std::size_t>(gameState.myPlayerData.getHand().getCards().size() - 1, 5)]);
                    discarded = true;
                }
            } else {
                //spdlog::info("[{}] Waiting for your turn...", playerName);
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

int main(int argc, char* argv[]) {
    if (argc < 2) {
        spdlog::error("Player index not specified!");
        return 1;
    }

    GameView gameView;
    CanastaConsole console;

    try {
        configureLogger(); // Configure the logger

        asio::io_context ioContext;

        // Connect to the server
        auto clientNetwork = std::make_shared<ClientNetwork>(ioContext);

        auto clientController = std::make_shared<ClientController>(clientNetwork, gameView);
        clientController->connect("127.0.0.1", std::to_string(SERVER_PORT));

        ioContext.run();
    } catch (const std::exception& e) {
        spdlog::error("Error: {}", e.what());
        return 1;
    }

    return 0;
}
