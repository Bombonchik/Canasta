#ifndef ROUND_MANAGER_HPP
#define ROUND_MANAGER_HPP

#include <vector>
#include <memory>
#include <optional>
#include <string>
#include <map>
#include <functional> // For std::reference_wrapper

#include "player.hpp"
#include "team.hpp"
#include "team_round_state.hpp"
#include "server/server_deck.hpp"
#include "card.hpp"
#include "turn_manager.hpp" // Includes TurnActionResult, MeldRequest etc.
#include "score_details.hpp"
#include "rule_engine.hpp" // For GameOutcome
#include "client_deck.hpp"
#include "player_public_info.hpp"


/**
 * @brief Manages the state and progression of a single round of Canasta.
 *
 * Owns the deck, discard pile, and team states for the round.
 * Orchestrates player turns using TurnManager. Calculates scores at round end.
 */
class RoundManager {
public:
    /**
     * @brief Constructs a RoundManager.
     * @param players Vector of all players participating in the round.
     * @param 
     * @param 
     */
    RoundManager(
        const std::vector<std::reference_wrapper<Player>>& players, // All players in turn order
        std::reference_wrapper<const Team> team1,
        std::reference_wrapper<const Team> team2
    );

    /**
     * @brief Initializes the round: deals cards, sets up discard pile, determines starting player.
     */
    void startRound();

    /**
     * @brief Gets the ID or reference of the player whose turn it currently is.
     * @return Reference to the current player. Throws if round not started/over.
     */
    Player& getCurrentPlayer() const;

    // --- Action Handling (Delegates to current TurnManager) ---

    /**
     * @brief Handles a request from the current player to draw from the deck.
     * @return Result of the action from TurnManager.
     */
    TurnActionResult handleDrawDeckRequest();

    /**
     * @brief Handles a request from the current player to take the discard pile.
     * @return Result of the action from TurnManager.
     */
    TurnActionResult handleTakeDiscardPileRequest();

    /**
     * @brief Handles a request from the current player to meld cards.
     * @param meldRequests The list of meld actions the player wants to perform.
     * @return Result of the action from TurnManager.
     */
    TurnActionResult handleMeldRequest(const std::vector<MeldRequest>& meldRequests);

    /**
     * @brief Handles a request from the current player to discard a card.
     * @param cardToDiscard The card the player wants to discard.
     * @return Result of the action from TurnManager.
     */
    TurnActionResult handleDiscardRequest(const Card& cardToDiscard);

    /**
     * @brief Handles a request from the current player to revert provisional actions.
     * (Only applicable if TurnManager supports it, e.g., before initial meld commit)
     * @return Result of the action from TurnManager.
     */
    TurnActionResult handleRevertRequest();


    /**
     * @brief Checks if the round has finished.
     * @return True if the round is over, false otherwise.
     */
    bool isRoundOver() const;

    /**
     * @brief Calculates the scores for each team at the end of the round.
     * Should only be called when isRoundOver() is true.
     * @return A map where the key is the team name (or ID) and the value is the ScoreBreakdown.
     */
    std::map<std::string, ScoreBreakdown> calculateScores() const;

    ClientDeck getClientDeck() const;

    std::vector<PlayerPublicInfo> getAllPlayersPublicInfo() const;

    TeamRoundState getTeamStateForTeam(const Team& team) const;

private:

    static constexpr std::size_t INITIAL_HAND_SIZE = 11; // Number of cards dealt to each player at the start
    enum class RoundPhase {
        NotStarted,
        Dealing,
        InProgress,
        Finished
    };

    // --- Internal State ---
    const std::vector<std::reference_wrapper<Player>> playersInTurnOrder;
    std::reference_wrapper<const Team> team1; // Store for lookup
    std::reference_wrapper<const Team> team2; // Store for lookup
    TeamRoundState team1State;
    TeamRoundState team2State;

    RoundPhase roundPhase;
    ServerDeck serverDeck;
    std::size_t currentPlayerIndex;
    std::optional<std::reference_wrapper<Player>> playerWhoWentOut; // Store who went out
    bool isMainDeckEmpty;

    std::unique_ptr<TurnManager> currentTurnManager;

    // --- Private Helpers ---

    /**
     * @brief Deals initial hands to all players.
     */
    void dealInitialHands();

    /**
     * @brief Creates and prepares the TurnManager for the current player.
     */
    void setupTurnManagerForCurrentPlayer();

    /**
     * @brief Processes the result from TurnManager, advances turn, checks round end etc.
     * @param result The result returned by the TurnManager action handler.
     */
    void processTurnResult(const TurnActionResult& result);

    /**
     * @brief Advances to the next player's turn.
     */
    void advanceToNextPlayer();

    const Team& getTeamForPlayer(const Player& player) const;

    /**
     * @brief Gets the TeamRoundState associated with a given player. Helper function.
     */
    TeamRoundState& getTeamStateForPlayer(Player& player);
    const TeamRoundState& getTeamStateForPlayer(const Player& player) const; // Const overload
};

#endif // ROUND_MANAGER_HPP