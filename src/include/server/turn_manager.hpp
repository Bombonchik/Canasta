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
     * @param playerTeam Reference to the current player's team (for melds).
     * @param serverDeck Reference to the sever's deck
     * @param teamAlreadyHasInitialMeld True if the player's team has already met the initial meld requirement this round.
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
     * @return TurnActionResult indicating success or failure.
     */
    TurnActionResult handleDrawDeck();

    /**
     * @brief Handles the player's attempt to take the discard pile.
     * If the team hasn't made the initial meld, this is provisional until discard.
     * @return TurnActionResult indicating success, failure, or provisional success.
     */
    TurnActionResult handleTakeDiscardPile();

    /**
     * @brief Handles the player's attempt to meld cards.
     * Can only be called during the MeldDiscard phase.
     * If the team hasn't made the initial meld, these melds are provisional until discard.
     * @param proposals A vector of meld proposals.
     * @return TurnActionResult indicating success or failure of the meld attempt(s).
     */
    TurnActionResult handleMelds(const std::vector<MeldRequest>& meldRequests);

    /**
     * @brief Handles the player's attempt to discard a card to end the turn.
     * Can only be called during the MeldDiscard phase. Triggers final validation
     * if the team hasn't made the initial meld yet.
     * @param cardToDiscard The card the player wishes to discard.
     * @return TurnActionResult indicating success (turn over/went out), failure (invalid discard),
     *         or the special revert case (Error_RevertAndForceDraw or Error_MeldRequirementNotMet).
     */
    TurnActionResult handleDiscard(const Card& cardToDiscard);


    TurnActionResult handleRevert();
private:

    std::reference_wrapper<Player> player; // Reference to the player whose turn it is
    std::reference_wrapper<Hand> hand; // Reference to the player's hand
    std::reference_wrapper<TeamRoundState> teamRoundState;
    std::reference_wrapper<ServerDeck> serverDeck;

    // Turn-specific state flags and data
    const bool teamHasInitialRankMeld;

    int teamTotalScore; // Total score of the player's team

    bool drewFromDeck; // Tracks if the player drew from the deck
    bool tookDiscardPile; // Tracks if the player took the discard pile
    bool meldsHandled; // Tracks if handleMelds was successfully executed
    std::optional<MeldCommitment> commitment;
    std::vector<RankMeldProposal> rankInitializationProposals;
    std::vector<RankMeldProposal> rankAdditionProposals;


    // --- Private Helper Methods ---
    
    /// Checks if all cards from Meld Requests are presented in the player's hand
    /// Returns an error message if any card is missing,
    /// or the number of cards that will remain in the hand if the melds are correct
    std::expected<std::size_t, TurnActionResult> checkMeldRequestsCardsInHand
    (const std::vector<MeldRequest>& meldRequests) const;

    void processMeldRequests(
        const std::vector<MeldRequest>& meldRequests,
        std::vector<std::vector<Card>>& meldSuggestions,
        std::vector<RankMeldProposal>& additionProposals
    );

    std::expected<void, TurnActionResult> processMeldSuggestions
    (
        const std::vector<std::vector<Card>>& meldSuggestions, 
        std::vector<RankMeldProposal>& rankInitializationProposals,
        std::optional<BlackThreeMeldProposal>& blackThreeProposal
    );

    std::expected<void, TurnActionResult> processRankInitializationProposals
    (const std::vector<RankMeldProposal>& rankInitializationProposals) const;

    std::expected<void, TurnActionResult> processBlackThreeInitializationProposal
    (const std::optional<BlackThreeMeldProposal>& blackThreeProposal,
        bool canGoingOut) const;

    std::expected<void, TurnActionResult> processRankAdditionProposals
    (const std::vector<RankMeldProposal>& rankAdditionProposals) const;

    std::expected<Card, TurnActionResult> drawUntilNonRedThree(ServerDeck& deck);

    std::expected<void, TurnActionResult> checkInitializationCommitment
    (const std::vector<RankMeldProposal>& initializationProposals,
        const MeldCommitment& initializationCommitment) const;

    std::expected<void, TurnActionResult> checkAddToExistingCommitment
    (const std::vector<RankMeldProposal>& additionProposals,
        const MeldCommitment& AddToExistingCommitment) const;

    void initializeRankMelds
    (const std::vector<RankMeldProposal>& rankInitializationProposals);

    void revertRankMeldsInitialization
    (const std::vector<RankMeldProposal>& rankInitializationProposals);

    void addCardsToExistingMelds
    (const std::vector<RankMeldProposal>& additionProposals);

    void revertRankMeldsAddition
    (const std::vector<RankMeldProposal>& additionProposals);

    void initializeBlackThreeMeld
    (const std::optional<BlackThreeMeldProposal>& blackThreeProposal);

    // Performs the mandatory revert when initial meld fails after taking the pile
    void revertTakeDiscardPileAction();

    void revertRankMeldActions(const std::vector<RankMeldProposal>& rankInitializationProposals,
        const std::vector<RankMeldProposal>& additionProposals);

    void clearProposals();
};


#endif // TURN_MANAGER_HPP