#include <spdlog/spdlog.h>
#include "server/game_manager.hpp"
#include "player.hpp"
#include "team.hpp"
#include "score_details.hpp" // Included via round_manager? Explicitly add if needed.

#include <stdexcept>
#include <numeric> // For std::iota potentially
#include <algorithm> // For std::shuffle potentially

// --- Constructor ---

GameManager::GameManager(std::size_t playersCount)
    :   playersCount(playersCount),
        team1("Team 1"), // Initialize teams
        team2("Team 2"),
        gamePhase(GamePhase::NotStarted),
        finalOutcome(std::nullopt)
{
    if (playersCount != 4 && playersCount != 2) {
        throw std::invalid_argument("GameManager currently requires 2 or 4 players.");
    }
}

// --- Public Methods ---

bool GameManager::allPlayersJoined() const {
    return playersCount == playerNames.size();
}

Status GameManager::addPlayer(const std::string& playerName) {
    if (gamePhase != GamePhase::NotStarted) {
        return std::unexpected("Game has already started or is finished.");
    }
    if (std::find(playerNames.begin(), playerNames.end(), playerName) != playerNames.end()) {
        return std::unexpected("Player name already exists.");
    }
    if (allPlayersJoined()) {
        return std::unexpected("Game is full. Cannot add more players.");
    }

    playerNames.push_back(playerName);
    spdlog::info("Player {} added. Total players: {}", playerName, playerNames.size());

    // If we have enough players, set up teams
    if (allPlayersJoined()) {
        setupTeams();
    }
    return {};
}

void GameManager::startGame() {
    if (gamePhase != GamePhase::NotStarted) {
        throw std::logic_error("Game can only be started once.");
    }
    spdlog::info("Starting game with {} players.", playersCount);
    startNextRound(); // Start the first round
}

bool GameManager::isGameOver() const {
    return gamePhase == GamePhase::Finished;
}

std::optional<GameOutcome> GameManager::getGameOutcome() const {
    return finalOutcome;
}

std::optional<std::reference_wrapper<const Team>> GameManager::getWinningTeam() const {
    if (!isGameOver() || !finalOutcome.has_value()) {
        return std::nullopt;
    }

    switch (*finalOutcome) {
        case GameOutcome::Team1Wins:
            return std::cref(team1);
        case GameOutcome::Team2Wins:
            return std::cref(team2);
        case GameOutcome::Continue: // Should not happen if game is over
        case GameOutcome::Draw:
        default:
            return std::nullopt; // No single winner in draw or error state
    }
}

RoundManager* GameManager::getCurrentRoundManager() {
    return currentRound.get();
}

const RoundManager* GameManager::getCurrentRoundManager() const {
    return currentRound.get();
}

void GameManager::advanceGameState() {
    if (gamePhase == GamePhase::RoundInProgress && currentRound) {
        // Check if the current round has naturally concluded
        if (currentRound->isRoundOver()) {
            spdlog::info("Round is over. Handling round completion...");
            handleRoundCompletion();
        }
        // If round is not over, do nothing - wait for player actions via RoundManager
    } else if (gamePhase == GamePhase::BetweenRounds) {
        // If game is in between rounds, we can start the next round
        if (!isGameOver()) {
            //gamePhase = GamePhase::BetweenRounds;
            spdlog::info("Game continues. Ready to start next round.");
            // Optionally add a delay or wait for user input here
            startNextRound();
        }
    }
    // No advancement needed if NotStarted or Finished
}


const Team& GameManager::getTeam1() const {
    return team1;
}

const Team& GameManager::getTeam2() const {
    return team2;
}

const std::vector<Player>& GameManager::getAllPlayers() const {
    return allPlayers;
}

void GameManager::handlePlayerDisconnect(const std::string& playerName) {
    // in current implementation, we just log the disconnection and kill the game
    spdlog::info("Player {} disconnected. Ending game.", playerName);
}


// --- Private Helpers ---

void GameManager::setupTeams() {
    spdlog::debug("Setting up teams...");
    allPlayers.clear();
    // Create Player objects - GameManager owns them
    for (const auto& name : playerNames) {
        allPlayers.emplace_back(name);
    }

    for (std::size_t i = 0; i < playersCount; i += 2) {
        team1.addPlayer(allPlayers[i]);
        team2.addPlayer(allPlayers[i + 1]);
    }
    spdlog::info("Teams setup: {} vs {}", team1.getName(), team2.getName());
}

void GameManager::startNextRound() {
    if (gamePhase == GamePhase::Finished) {
        spdlog::error("Cannot start a new round: game is already finished.");
        return;
    }

    spdlog::info("Starting a new round...");
    // Create player references for RoundManager constructor
    std::vector<std::reference_wrapper<Player>> playerRefs;
    for (Player& p : allPlayers) {
        playerRefs.emplace_back(p);
    }
    RuleEngine::randomRotate(playerRefs);

    currentRound.reset(); // Reset any existing round manager
    currentRound = std::make_unique<RoundManager>(
        playerRefs,
        std::cref(team1), // Pass const reference to Team
        std::cref(team2)
    );

    currentRound->startRound();
    gamePhase = GamePhase::RoundInProgress;
}

void GameManager::handleRoundCompletion() {
    if (!currentRound || !currentRound->isRoundOver()) {
        spdlog::error("Cannot handle round completion: no round in progress or round not over.");
        return;
    }

    spdlog::info("Calculating round scores...");
    std::map<std::string, ScoreBreakdown> scores = currentRound->calculateScores();

    // Update total scores in Team objects
    // Need to handle potential key errors if team names don't match exactly
    try {
        int team1RoundScore = scores[team1.getName()].calculateTotal();
        int team2RoundScore = scores[team2.getName()].calculateTotal();
        team1.addToTotalScore(team1RoundScore);
        team2.addToTotalScore(team2RoundScore);
        spdlog::info("{} round score: {}, total: {}", team1.getName(), team1RoundScore, team1.getTotalScore());
        spdlog::info("{} round score: {}, total: {}", team2.getName(), team2RoundScore, team2.getTotalScore());

    } catch (const std::out_of_range& oor) {
        spdlog::error("Error calculating scores: {}", oor.what());
        throw std::logic_error("Team names do not match the expected format.");
    } catch (const std::exception& e) {
        spdlog::error("Unexpected error while calculating scores: {}", e.what());
        throw; 
    }

    // Check game outcome based on new total scores
    GameOutcome outcome = RuleEngine::checkGameOutcome(team1.getTotalScore(), team2.getTotalScore());
    finalOutcome = outcome; // Store the outcome

    if (outcome == GameOutcome::Continue) {
        gamePhase = GamePhase::BetweenRounds; // Ready for next round
    } else {
        gamePhase = GamePhase::Finished; // Game over
        auto message = (outcome == GameOutcome::Draw) ? "It's a draw!" :
            (outcome == GameOutcome::Team1Wins) ? "Team 1 wins!" : "Team 2 wins!";
        spdlog::info("Game over! Outcome: {}", message);
    }
}

const Player& GameManager::getPlayerByName(const std::string& name) const {
    auto it = std::find_if(allPlayers.begin(), allPlayers.end(),
        [&name](const Player& player) { return player.getName() == name; });

    if (it != allPlayers.end()) {
        return *it;
    } else {
        throw std::invalid_argument("Player with name " + name + " not found.");
    }
}