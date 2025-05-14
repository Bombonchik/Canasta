#ifndef TURN_MANAGER_HPP
#define TURN_MANAGER_HPP

#include <vector>
#include <memory>
#include <optional>
#include <string> // For error messages potentially in results
#include <cereal/types/vector.hpp>   // Needed for vector serialization
#include <cereal/types/optional.hpp> // Needed for optional serialization
#include "card.hpp"
#include "hand.hpp"
#include "player.hpp"
#include "team_round_state.hpp"
#include "server/server_deck.hpp"
#include "rule_engine.hpp" // For MeldProposal definition and static methods


/**
 * @class TurnManager
 * @brief Manages the state and logic for a single player's turn.
 *
 * This class acts as a service, taking references to the relevant game state
 * components and providing methods to handle player actions. It enforces turn phases
 * and handles the specific Canasta rules
 */
class TurnManager {
public:
    /**
     * @brief Constructs a TurnManager for a player's turn.
     * @param player Reference to the player whose turn it is.
     * @param teamRoundState Reference to the team's round state.
     * @param serverDeck Reference to the server deck.
     * @param teamAlreadyHasInitialMeld True if the player's team has already met the initial meld requirement this round.
     * @param teamTotalScore The total score of the player's team.
     */
    TurnManager(
        Player& player,
        TeamRoundState& teamRoundState,
        ServerDeck& serverDeck,
        bool teamAlreadyHasInitialMeld,
        int teamTotalScore
    );

    // --- Player Action Handlers ---

    /**
     * @brief Handles the player's action to draw a card from the deck.
     * @details This method can only be called during the Draw Card / Take Discard Pile phase.
     * This action isn't reversible.
     * @return TurnActionResult indicating success or failure.
     */
    TurnActionResult handleDrawDeck();

    /**
     * @brief Handles the player's attempt to take the discard pile.
     * @details This method can only be called during the Draw Card / Take Discard Pile phase.
     * Even if the player took the pile successfully they could revert this action by the end of the turn.
     * @return TurnActionResult indicating success or failure.
     */
    TurnActionResult handleTakeDiscardPile();

    /**
     * @brief Handles the player's attempt to meld cards.
     * @details Can only be called during the MeldDiscard phase.
     * This method processes the meld requests and updates the player's hand and team state.
     * Even if the player melded successfully they could revert this action by the end of the turn.
     * @param proposals A vector of meld proposals.
     * @return TurnActionResult indicating success or failure of the meld attempt(s).
     */
    TurnActionResult handleMelds(const std::vector<MeldRequest>& meldRequests);

    /**
     * @brief Handles the player's attempt to discard a card to end the turn.
     * @details Can only be called during the MeldDiscard phase. Triggers final validations
     * This action isn't reversible.
     * @param cardToDiscard The card the player wishes to discard.
     * @return TurnActionResult indicating success (turn over/went out), failure (invalid discard),
     */
    TurnActionResult handleDiscard(const Card& cardToDiscard);


    /**
     * @brief Handles the player's request to revert provisional actions.
     * @details This method can only be called if the player has taken the discard pile or handled melds.
     * @return TurnActionResult indicating success or failure of the revert action.
     */
    TurnActionResult handleRevert();
private:

    std::reference_wrapper<Player> player; // Reference to the player whose turn it is
    std::reference_wrapper<Hand> hand; // Reference to the player's hand
    std::reference_wrapper<TeamRoundState> teamRoundState;
    std::reference_wrapper<ServerDeck> serverDeck;

    // Turn-specific state flags and data
    const bool teamHasInitialRankMeld;

    int teamTotalScore; // Total score of the player's team

    bool drewFromDeck; ///< Tracks if the player drew from the deck
    bool tookDiscardPile; ///< Tracks if the player took the discard pile
    bool meldsHandled; ///< Tracks if handleMelds was successfully executed
    std::optional<MeldCommitment> commitment; ///< Commitment to the melds
    std::vector<RankMeldProposal> rankInitializationProposals; ///< Proposals for initializing rank melds
    std::vector<RankMeldProposal> rankAdditionProposals; ///< Proposals for adding cards to existing rank melds


    // --- Private Helper Methods ---
    /**
     * @brief Checks if all cards from Meld Requests are presented in the player's hand
     * @return an error message if any card is missing,
     * or the number of cards that will remain in the hand if the melds are correct
     */
    std::expected<std::size_t, TurnActionResult> checkMeldRequestsCardsInHand
    (const std::vector<MeldRequest>& meldRequests) const;

    /**
     * @brief Processes the meld requests splitting them into suggestions and addition proposals.
     * @param meldRequests The list of meld requests from the player.
     * @param meldSuggestions The generated meld suggestions.
     * @param additionProposals The proposals for adding cards to existing melds.
     */
    void processMeldRequests(
        const std::vector<MeldRequest>& meldRequests,
        std::vector<std::vector<Card>>& meldSuggestions,
        std::vector<RankMeldProposal>& additionProposals
    );

    /**
     * @brief Processes the meld suggestions and generates initialization proposals.
     * @param meldSuggestions The generated meld suggestions.
     * @param rankInitializationProposals The proposals for initializing rank melds.
     * @param blackThreeProposal The proposal for initializing a Black Three meld.
     * @return An error message if any issue occurs, or void on success.
     */
    std::expected<void, TurnActionResult> processMeldSuggestions(
        const std::vector<std::vector<Card>>& meldSuggestions, 
        std::vector<RankMeldProposal>& rankInitializationProposals,
        std::optional<BlackThreeMeldProposal>& blackThreeProposal
    );

    /**
     * @brief Processes the rank initialization proposals and validates them.
     * @param rankInitializationProposals The proposals for initializing rank melds.
     * @return An error message if any issue occurs, or void on success.
     */
    std::expected<void, TurnActionResult> processRankInitializationProposals
    (const std::vector<RankMeldProposal>& rankInitializationProposals) const;

    /**
     * @brief Processes the Black Three initialization proposal and validates it.
     * @param blackThreeProposal The proposal for initializing a Black Three meld.
     * @param canGoingOut Indicates if the player can go out this turn.
     * @return An error message if any issue occurs, or void on success.
     */
    std::expected<void, TurnActionResult> processBlackThreeInitializationProposal
    (const std::optional<BlackThreeMeldProposal>& blackThreeProposal,
        bool canGoingOut) const;

    /**
     * @brief Processes the rank addition proposals and validates them.
     * @param rankAdditionProposals The proposals for adding cards to existing melds.
     * @return An error message if any issue occurs, or void on success.
     */
    std::expected<void, TurnActionResult> processRankAdditionProposals
    (const std::vector<RankMeldProposal>& rankAdditionProposals) const;

    /**
     * @brief Draws cards from the deck until a non-red three is found.
     * @details This method is used to handle the case where the player draws a red three.
     * It will keep drawing until a non-red three is found or the deck is empty.
     * @param deck The server deck to draw from.
     * @return The drawn card if successful, or an error message if the deck is empty.
     */
    std::expected<Card, TurnActionResult> drawUntilNonRedThree(ServerDeck& deck);

    /**
     * @brief Checks if the player fulfilled the commitment for the initialization of a meld after taking the discard pile.
     * @param initializationProposals The proposals for initializing rank melds.
     * @param initializationCommitment The commitment to be checked.
     * @return An error message if the commitment is not fulfilled, or void on success.
     */
    std::expected<void, TurnActionResult> checkInitializationCommitment
    (const std::vector<RankMeldProposal>& initializationProposals,
        const MeldCommitment& initializationCommitment) const;

    /**
     * @brief Checks if the player fulfilled the commitment for adding cards to an existing meld after taking the discard pile.
     * @param additionProposals The proposals for adding cards to existing melds.
     * @param AddToExistingCommitment The commitment to be checked.
     * @return An error message if the commitment is not fulfilled, or void on success.
     */
    std::expected<void, TurnActionResult> checkAddToExistingCommitment
    (const std::vector<RankMeldProposal>& additionProposals,
        const MeldCommitment& AddToExistingCommitment) const;

    /**
     * @brief Initializes the rank melds based on the proposals.
     * @param rankInitializationProposals The proposals for initializing rank melds.
     */
    void initializeRankMelds
    (const std::vector<RankMeldProposal>& rankInitializationProposals);

    /**
     * @brief Reverts the initialization of rank melds based on the proposals.
     * @param rankInitializationProposals The proposals for initializing rank melds.
     */
    void revertRankMeldsInitialization
    (const std::vector<RankMeldProposal>& rankInitializationProposals);

    /**
     * @brief Adds cards to existing melds based on the proposals.
     * @param additionProposals The proposals for adding cards to existing melds.
     */
    void addCardsToExistingMelds
    (const std::vector<RankMeldProposal>& additionProposals);

    /**
     * @brief Reverts the addition of cards to existing melds based on the proposals.
     * @param additionProposals The proposals for adding cards to existing melds.
     */
    void revertRankMeldsAddition
    (const std::vector<RankMeldProposal>& additionProposals);

    /**
     * @brief Initializes the Black Three meld based on the proposal.
     * @param blackThreeProposal The proposal for initializing a Black Three meld.
     */
    void initializeBlackThreeMeld
    (const std::optional<BlackThreeMeldProposal>& blackThreeProposal);

    /**
     * @brief Reverts taking the discard pile.
     */
    void revertTakeDiscardPileAction();

    /**
     * @brief Reverts the rank meld actions based on the proposals.
     * @param rankInitializationProposals The proposals for initializing rank melds.
     * @param additionProposals The proposals for adding cards to existing melds.
     */
    void revertRankMeldActions(const std::vector<RankMeldProposal>& rankInitializationProposals,
        const std::vector<RankMeldProposal>& additionProposals);

    /**
     * @brief Clears the proposals for rank initialization and addition.
     */
    void clearProposals();
};


#endif // TURN_MANAGER_HPP