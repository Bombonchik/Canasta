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
}

void ClientController::connect(const std::string& host, const std::string& port) {
    std::string playerNamePlaceholder = "Player"; // Default or last used name
    localPlayerName = view.promptString("Enter your player name:", playerNamePlaceholder);

    if (localPlayerName.empty())
        return;

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
}

void ClientController::resetTurnActionStatuses() {
    drawDeckAttemptStatus = ActionAttemptStatus::NotAttempted;
    takeDiscardPileAttemptStatus = ActionAttemptStatus::NotAttempted;
    meldAttemptStatus = ActionAttemptStatus::NotAttempted;
    discardAttemptStatus = ActionAttemptStatus::NotAttempted;
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
    auto myTeamState = gameState.getMyTeamState().clone();
    auto opponentTeamState = gameState.getOpponentTeamState().clone();
    boardState.setMyTeamMelds(getMeldViewsFromTeamRoundState(myTeamState)); // wrong temporary
    boardState.setOpponentTeamMelds(getMeldViewsFromTeamRoundState(opponentTeamState));
    boardState.setMyHand(gameState.getMyPlayerData().getHand());
    boardState.setDeckState(gameState.getDeckState());
    auto allPlayersInfo = gameState.getAllPlayersPublicInfo();
    boardState.setMyPlayer(allPlayersInfo[0]); // Assuming the first player is the current player
    if (allPlayersInfo.size() == RuleEngine::TWO_PLAYERS_GAME) {
        boardState.setOppositePlayer(allPlayersInfo[1]); // Assuming the second player is the opponent
    } else if (allPlayersInfo.size() == RuleEngine::FOUR_PLAYERS_GAME) {
        boardState.setLeftPlayer(allPlayersInfo[1]); // Assuming the second player is the left player
        boardState.setOppositePlayer(allPlayersInfo[2]); // Assuming the third player is the opponent
        boardState.setRightPlayer(allPlayersInfo[3]); // Assuming the fourth player is the right player
    }
    boardState.setMyTeamTotalScore(gameState.getMyTeamTotalScore());
    boardState.setOpponentTeamTotalScore(gameState.getOpponentTeamTotalScore());
    boardState.setMyTeamMeldPoints(myTeamState.calculateMeldPoints());
    boardState.setOpponentTeamMeldPoints(opponentTeamState.calculateMeldPoints());
    return boardState;
}

ScoreState ClientController::getScoreState(const ClientGameState& gameState) const {
    ScoreState scoreState;
    scoreState.setMyTeamScoreBreakdown(gameState.getMyTeamScoreBreakdown().value());
    scoreState.setOpponentTeamScoreBreakdown(gameState.getOpponentTeamScoreBreakdown().value());
    scoreState.setPlayersCount(gameState.getAllPlayersPublicInfo().size());
    scoreState.setMyTeamTotalScore(gameState.getMyTeamTotalScore());
    scoreState.setOpponentTeamTotalScore(gameState.getOpponentTeamTotalScore());
    scoreState.setIsGameOver(gameState.getIsGameOver());
    scoreState.setGameOutcome(gameState.getGameOutcome());
    return scoreState;
}

// --- Callback Handlers from ClientNetwork (Stubs) ---
void ClientController::handleGameStateUpdate(const ClientGameState& gameState) {
    bool isMyTurn = gameState.getAllPlayersPublicInfo()[0].isCurrentPlayer();
    currentBoardState = getBoardState(gameState);
    auto status = gameState.getStatus();
    if (isMyTurn &&
        !status.has_value() || // Turn started
        (status.has_value() && status.value() == TurnActionStatus::Success_TurnContinues)) {
        processPlayerTurn();
    } else {
        resetTurnActionStatuses();
        if (gameState.getIsRoundOver()) {
            view.showStaticScore(getScoreState(gameState));
            if (!gameState.getIsGameOver())
                std::this_thread::sleep_for(std::chrono::seconds(SCORE_TIME));
        }
        else {
            view.showStaticBoardWithMessages({gameState.getLastActionDescription()}, currentBoardState);
        }
    }
}

void ClientController::handleActionError(const ActionError& error) {
    if (!error.getStatus().has_value()) {
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

    view.restoreInput();
    if (meldAttemptStatus == ActionAttemptStatus::Succeeded) {
        return processAfterMelding(error.getMessage());
    } else if (takeDiscardPileAttemptStatus == ActionAttemptStatus::Succeeded) {
        return processAfterTakingDiscardPile(error.getMessage());
    } else if (drawDeckAttemptStatus == ActionAttemptStatus::Succeeded) {
        return processAfterDrawing(error.getMessage());
    } else if (discardAttemptStatus == ActionAttemptStatus::Succeeded) {
        throw std::runtime_error("Discard attempt status should not be succeeded here.");
    } else if (drawDeckAttemptStatus == ActionAttemptStatus::NotAttempted && 
                takeDiscardPileAttemptStatus == ActionAttemptStatus::NotAttempted) {
        return promptAndProcessDrawCardOrTakeDiscardPile(error.getMessage());
    }
}

void ClientController::handleLoginSuccess() {
    resetTurnActionStatuses(); // Good place to ensure fresh start
}

void ClientController::handleLoginFailure(const std::string& reason) {
    std::cout << "[ClientController] Login Failed: " << reason << std::endl; // Placeholder
    localPlayerName = ""; // Clear player name on login failure
}

void ClientController::handleDisconnect() {
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
    
    auto myTeamMelds = currentBoardState.getMyTeamMelds();
    for (auto& meldRequest : meldRequests) {
        auto meldRequestRank = meldRequest.getRank();
        if (meldRequestRank.has_value() && meldRequestRank.value() >= Rank::Four) {
            const auto& meld = myTeamMelds[BoardState::getMeldIndexForRank(meldRequestRank.value()).value()];
            if (!meld.isInitialized())
                meldRequest.setRank(std::nullopt); // Reset to nullopt if not initialized
        }
    }

    meldAttemptStatus = ActionAttemptStatus::Attempting;
    network->sendMeld(meldRequests);
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
}

void ClientController::processRevert() {
    takeDiscardPileAttemptStatus = ActionAttemptStatus::NotAttempted;
    meldAttemptStatus = ActionAttemptStatus::NotAttempted;
    discardAttemptStatus = ActionAttemptStatus::NotAttempted;
    network->sendRevert();
}