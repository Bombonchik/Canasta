#ifndef GAME_MANAGER_HPP
#define GAME_MANAGER_HPP

#include <vector>
#include <string>
#include <memory>
#include <optional>
#include <map>
#include <functional> // For std::reference_wrapper

#include "player.hpp"
#include "team.hpp"
#include "server/round_manager.hpp" // Manages a single round
#include "rule_engine.hpp"   // For GameOutcome and WINNING_SCORE


/**
 * @class GameManager
 * @brief Orchestrates the overall Canasta game, managing players, teams,
 * multiple rounds, and total scores.
 */
class GameManager {
public:
    /**
     * @brief Constructs a GameManager.
     * @param playersCount Number of players in the game (2 or 4).
     */
    explicit GameManager(std::size_t playersCount);

    /**
     * @brief Checks if number of joined players is equal to playersCount.
     * @return True if all players have joined, false otherwise.
     */
    bool allPlayersJoined() const;

    /**
     * @brief Adds a player to the game.
     * @param playerName Name of the player to add.
     * @return Status indicating success or failure.
     */
    Status addPlayer(const std::string& playerName);

    /**
     * @brief Starts the game, dealing the first round.
     * Throws std::logic_error if the game is already started.
     */
    void startGame();

    /**
     * @brief Checks if the entire game (multiple rounds) is over.
     */
    bool isGameOver() const;

    /**
     * @brief Gets the outcome of the game if it's over.
     * @return GameOutcome enum value, or std::nullopt if the game is not over.
     */
    std::optional<GameOutcome> getGameOutcome() const;

    /**
     * @brief Gets the winning team, if the game is over and there's a winner.
     * @return Optional reference to the winning Team object. Returns nullopt if draw or game not over.
     */
    std::optional<std::reference_wrapper<const Team>> getWinningTeam() const;

    /**
     * @brief Gets the current round manager, if a round is in progress.
     * Useful for querying round-specific state or passing actions down.
     * @return Pointer to the current RoundManager, or nullptr if no round is active.
     */
    RoundManager* getCurrentRoundManager();
    const RoundManager* getCurrentRoundManager() const; // Const overload

    /**
     * @brief Advances the game state.
     * Checks if the current round is over and starts the next one if needed.
     */
    void advanceGameState();

    // --- Getters for Game State ---
    const Team& getTeam1() const;
    const Team& getTeam2() const;
    const std::vector<Player>& getAllPlayers() const;

    const Player& getPlayerByName(const std::string& name) const;

    void handlePlayerDisconnect(const std::string& playerName);

private:
    /**
     * @brief Enum representing the phases of the game.
     */
    enum class GamePhase {
        NotStarted,
        RoundInProgress,
        BetweenRounds,
        Finished
    };

    // --- Game Components ---
    std::vector<Player> allPlayers; ///< Owns the Player objects
    std::size_t playersCount;
    Team team1;
    Team team2;

    // --- Game State ---
    GamePhase gamePhase;
    /// Final outcome of the game, if applicable
    std::optional<GameOutcome> finalOutcome;

    std::vector<std::string> playerNames; // Names of players in the game
    /// Unique pointer to the current round manager
    std::unique_ptr<RoundManager> currentRound;

    // --- Private Helpers ---

    /**
     * @brief Sets up players and teams based on initial names.
     * @param playerNames Names provided to the constructor.
     */
    void setupTeams();

    /**
     * @brief Starts a new round of play.
     */
    void startNextRound();

    /**
     * @brief Handles the completion of a round, updates scores, checks game end.
     */
    void handleRoundCompletion();

};

#endif // GAME_MANAGER_HPP