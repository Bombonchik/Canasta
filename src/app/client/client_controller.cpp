#include "client/client_controller.hpp"
#include "client/client_network.hpp" // Already included via client_controller.hpp but good for clarity
#include "client/game_view.hpp"     // Already included via client_controller.hpp
#include <functional>              // For std::bind / lambdas
#include <iostream>                // For placeholder messages, to be replaced by GameView calls
#include <string>                 // For std::string

ClientController::ClientController(std::shared_ptr<ClientNetwork> clientNetwork, GameView& gameView)
    : network(clientNetwork), view(gameView), localPlayerName("") {
    resetTurnActionStatuses();
    setupNetworkCallbacks();
    // std::cout << "ClientController initialized." << std::endl; // Placeholder
}

void ClientController::connect(const std::string& host, const std::string& port) {
    // std::cout << "Attempting to connect to " << host << ":" << port << std::endl; // Placeholder
    std::string playerNamePlaceholder = "Player"; // Default or last used name
    localPlayerName = view.promptString("Enter your player name:", playerNamePlaceholder);

    if (localPlayerName.empty()) {
        //std::cout << "Connection cancelled: Player name cannot be empty." << std::endl; // Placeholder
        // Potentially re-prompt or offer to exit
        return;
    }

    // view.showStaticMessage(fmt::format("Connecting as {}...", localPlayerName)); // Placeholder
    network->connect(host, port, localPlayerName);
}

void ClientController::setupNetworkCallbacks() {
    network->setOnGameStateUpdate(
        [this](const ClientGameState& gs) { this->handleGameStateUpdate(gs); });
    network->setOnActionError(
        [this](const ActionError& err) { this->handleActionError(err); });
    network->setOnLoginSuccess(
        [this]() { this->handleLoginSuccess(); });
    network->setOnLoginFailure(
        [this](const std::string& reason) { this->handleLoginFailure(reason); });
    network->setOnDisconnect(
        [this]() { this->handleDisconnect(); });
    // std::cout << "ClientNetwork callbacks set up." << std::endl; // Placeholder
}

void ClientController::resetTurnActionStatuses() {
    drawDeckAttemptStatus = ActionAttemptStatus::NotAttempted;
    takeDiscardPileAttemptStatus = ActionAttemptStatus::NotAttempted;
    meldAttemptStatus = ActionAttemptStatus::NotAttempted;
    discardAttemptStatus = ActionAttemptStatus::NotAttempted;
    // std::cout << "Turn action statuses reset." << std::endl; // Placeholder
}

std::vector<MeldView> ClientController::getMeldViewsFromTeamRoundState(const TeamRoundState& teamRoundState) const {
    std::vector<MeldView> meldViews;
    auto redThreeMeld = teamRoundState.getRedThreeMeld();
    if (redThreeMeld->isInitialized())
        meldViews.push_back({Rank::Three, redThreeMeld->getCards(), true});
    else
        meldViews.push_back({Rank::Three, {}, false});
    auto blackThreeMeld = teamRoundState.getBlackThreeMeld();
    if (blackThreeMeld->isInitialized())
        meldViews.push_back({Rank::Three, blackThreeMeld->getCards(), true});
    else
        meldViews.push_back({Rank::Three, {}, false});
    for (int r = static_cast<int>(Rank::Four); r <= static_cast<int>(Rank::Ace); ++r) {
        auto rank = static_cast<Rank>(r);
        auto meld = teamRoundState.getMeldForRank(rank);
        if (meld->isInitialized())
            meldViews.push_back({rank, meld->getCards(), true});
        else
            meldViews.push_back({rank, {}, false});
    }
    return meldViews;
}

BoardState ClientController::getBoardState(const ClientGameState& gameState) const {
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

ScoreState ClientController::getScoreState(const ClientGameState& gameState) const {
    ScoreState scoreState;
    scoreState.myTeamScoreBreakdown = gameState.myTeamScoreBreakdown.value();
    scoreState.opponentTeamScoreBreakdown = gameState.opponentTeamScoreBreakdown.value();
    scoreState.playersCount = gameState.allPlayersPublicInfo.size();
    scoreState.myTeamTotalScore = gameState.myTeamTotalScore;
    scoreState.opponentTeamTotalScore = gameState.opponentTeamTotalScore;
    scoreState.isGameOver = gameState.isGameOver;
    scoreState.gameOutcome = gameState.gameOutcome;
    return scoreState;
}

// --- Callback Handlers from ClientNetwork (Stubs) ---
void ClientController::handleGameStateUpdate(const ClientGameState& gameState) {
    bool isMyTurn = gameState.allPlayersPublicInfo[0].isCurrentTurn;
    currentBoardState = getBoardState(gameState);
    if (isMyTurn &&
        !gameState.status.has_value() || // Turn started
        (gameState.status.has_value() && gameState.status.value() == TurnActionStatus::Success_TurnContinues)) {
        processPlayerTurn();
    } else {
        resetTurnActionStatuses();
        if (gameState.isRoundOver) {
            view.showStaticScore(getScoreState(gameState));
            if (!gameState.isGameOver)
                std::this_thread::sleep_for(std::chrono::seconds(25));
        }
        else {
            view.showStaticBoardWithMessages({gameState.lastActionDescription}, currentBoardState);
        }
    }
}

void ClientController::handleActionError(const ActionError& error) {
    if (!error.status.has_value()) {
        // Should not happen
        throw std::runtime_error("ActionError without status.");    
    }
    if (drawDeckAttemptStatus == ActionAttemptStatus::Attempting) {
        drawDeckAttemptStatus = ActionAttemptStatus::NotAttempted;
    } else if (takeDiscardPileAttemptStatus == ActionAttemptStatus::Attempting) {
        takeDiscardPileAttemptStatus = ActionAttemptStatus::NotAttempted;
    } else if (meldAttemptStatus == ActionAttemptStatus::Attempting) {
        meldAttemptStatus = ActionAttemptStatus::NotAttempted;
    } else if (discardAttemptStatus == ActionAttemptStatus::Attempting) {
        discardAttemptStatus = ActionAttemptStatus::NotAttempted;
    } else {
        throw std::runtime_error("ActionError occurred but no action was in progress.");
    }
    //processPlayerTurn(error.message);

    view.restoreInput();
    if (meldAttemptStatus == ActionAttemptStatus::Succeeded) {
        return processAfterMelding(error.message);
    } else if (takeDiscardPileAttemptStatus == ActionAttemptStatus::Succeeded) {
        return processAfterTakingDiscardPile(error.message);
    } else if (drawDeckAttemptStatus == ActionAttemptStatus::Succeeded) {
        return processAfterDrawing(error.message);
    } else if (discardAttemptStatus == ActionAttemptStatus::Succeeded) {
        throw std::runtime_error("Discard attempt status should not be succeeded here.");
    } else if (drawDeckAttemptStatus == ActionAttemptStatus::NotAttempted && 
                takeDiscardPileAttemptStatus == ActionAttemptStatus::NotAttempted) {
        return promptAndProcessDrawCardOrTakeDiscardPile(error.message);
    }
}

void ClientController::handleLoginSuccess() {
    //std::cout << "[ClientController] Login Successful for " << localPlayerName << std::endl; // Placeholder
    resetTurnActionStatuses(); // Good place to ensure fresh start
}

void ClientController::handleLoginFailure(const std::string& reason) {
    // view.showError(fmt::format("Login failed: {}", reason));
    std::cout << "[ClientController] Login Failed: " << reason << std::endl; // Placeholder
    localPlayerName = ""; // Clear player name on login failure
}

void ClientController::handleDisconnect() {
    // view.showError("Disconnected from server.");
    std::cout << "[ClientController] Disconnected from server." << std::endl; // Placeholder
    resetTurnActionStatuses(); // Reset statuses
}

// --- Internal Game Logic (Stubs) ---
void ClientController::processPlayerTurn(std::optional<const std::string> message) {
    view.restoreInput();
    if (drawDeckAttemptStatus == ActionAttemptStatus::Attempting) {
        drawDeckAttemptStatus = ActionAttemptStatus::Succeeded; // Assuming draw was successful
        return processAfterDrawing(message);
    } else if (takeDiscardPileAttemptStatus == ActionAttemptStatus::Attempting) {
        takeDiscardPileAttemptStatus = ActionAttemptStatus::Succeeded; // Assuming take was successful
        return processAfterTakingDiscardPile(message);
    } else if (meldAttemptStatus == ActionAttemptStatus::Attempting) {
        meldAttemptStatus = ActionAttemptStatus::Succeeded; // Assuming meld was successful
        return processAfterMelding(message);
    } else if (discardAttemptStatus == ActionAttemptStatus::Attempting) { // Should not happen
        discardAttemptStatus = ActionAttemptStatus::Succeeded;
        throw std::runtime_error("Discard attempt status should not be attempting here.");
    } else if (drawDeckAttemptStatus == ActionAttemptStatus::Succeeded) { // After reset
        return processAfterDrawing(message);
    } else if (drawDeckAttemptStatus == ActionAttemptStatus::NotAttempted && 
                takeDiscardPileAttemptStatus == ActionAttemptStatus::NotAttempted) {
        return promptAndProcessDrawCardOrTakeDiscardPile(message);
    }
    std::cout << "[ClientController] Processing player turn for " << localPlayerName << std::endl; // Placeholder
}

void ClientController::promptAndProcessDrawCardOrTakeDiscardPile
    (std::optional<const std::string> message) {
    auto choise = view.promptChoiceWithBoard(
        "Choose an action:", {"Draw a card from deck", "Take discard pile"}, currentBoardState, message);
    if (choise == 0) { // Draw from deck
        drawDeckAttemptStatus = ActionAttemptStatus::Attempting;
        network->sendDrawDeck();
    } else {
        takeDiscardPileAttemptStatus = ActionAttemptStatus::Attempting;
        network->sendTakeDiscardPile();
    }
    //std::cout << "[ClientController] Prompting and processing draw." << std::endl; // Placeholder
}

void ClientController::processAfterDrawing(std::optional<const std::string> message) {
    auto choice = view.promptChoiceWithBoard(
        "Choose an action:", {"Melding", "Discard a card"}, currentBoardState, message);
    if (choice == 0) { // Melding
        processMelding(drawDeckAttemptStatus);
    } else { // Discard
        processDiscard();
    }
}

void ClientController::processAfterTakingDiscardPile(std::optional<const std::string> message) {
    auto choice = view.promptChoiceWithBoard(
        "Choose an action:", {"Melding", "Revert"}, currentBoardState, message);
    if (choice == 0) { // Melding
        processMelding(takeDiscardPileAttemptStatus);
    } else { // Discard
        processRevert();
    }
}

void ClientController::processMelding(ActionAttemptStatus& previousAttemptStatus) {
    std::vector<MeldRequest> meldRequests = view.runMeldWizard(currentBoardState);
    if (meldRequests.empty()){
        previousAttemptStatus = ActionAttemptStatus::Attempting;
        return processPlayerTurn(); // Go back
    }
    
    for (auto& meldRequest : meldRequests) {
        if (meldRequest.addToRank.has_value() && meldRequest.addToRank.value() >= Rank::Four) {
            const auto& meld = currentBoardState.myTeamMelds[static_cast<int>(meldRequest.addToRank.value()) - 2];
            if (!meld.isInitialized)
                meldRequest.addToRank = std::nullopt; // Reset to nullopt if not initialized
        }
    }

    meldAttemptStatus = ActionAttemptStatus::Attempting;
    network->sendMeld(meldRequests);
    //std::cout << "[ClientController] Processing meld." << std::endl; // Placeholder
}

void ClientController::processAfterMelding(std::optional<const std::string> message) {
    auto choice = view.promptChoiceWithBoard(
        "Choose an action:", {"Discard a card", "Revert"}, currentBoardState, message);
    if (choice == 0) { // Discard
        processDiscard();
    } else { // Revert
        processRevert();
    }
}

void ClientController::processDiscard() {
    Card cardToDiscard = view.runDiscardWizard(currentBoardState);
    discardAttemptStatus = ActionAttemptStatus::Attempting;
    network->sendDiscard(cardToDiscard);
    //std::cout << "[ClientController] Processing discard." << std::endl; // Placeholder
}

void ClientController::processRevert() {
    takeDiscardPileAttemptStatus = ActionAttemptStatus::NotAttempted;
    meldAttemptStatus = ActionAttemptStatus::NotAttempted;
    discardAttemptStatus = ActionAttemptStatus::NotAttempted;
    network->sendRevert();
    //std::cout << "[ClientController] Processing revert." << std::endl; // Placeholder
}