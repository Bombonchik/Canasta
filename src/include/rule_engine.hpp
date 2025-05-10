#ifndef RULE_ENGINE_HPP
#define RULE_ENGINE_HPP

#include <vector>
#include <string>
#include <optional>
#include <memory> // For unique_ptr
#include <expected> // Using std::expected for status/error
#include <functional> // For reference_wrapper
#include <variant>
#include <random>
#include <algorithm>
#include "hand.hpp"
#include "meld.hpp"
#include "team_round_state.hpp"

enum class CandidateMeldType {
    BlackThree,
    RankMeld
};

struct MeldSuggestion {
    CandidateMeldType type;
    // only set when it is normal Meld
    std::optional<Rank> rank;
};

struct RankMeldProposal {
    std::vector<Card>    cards;  // the exact cards to be melded
    Rank                 rank;   // Four…Ace
};

struct BlackThreeMeldProposal {
    std::vector<Card> cards; // the exact cards to be melded
};

enum class MeldCommitmentType {
    Initialize,  // initialize a new meld
    AddToExisting // add to an existing meld
};

struct MeldCommitment {
    MeldCommitmentType type;   // whether to initialize a new meld or add to an existing one
    Rank               rank;   // which rank the player is committing to
    std::size_t       count;   // how many cards of that rank should be used during turn
};

/// What happened at the end of the scoring phase?
enum class GameOutcome {
    Continue,     // Neither team has reached the threshold → start next round
    Team1Wins,    // Team 1 has more points and has reached the win threshold
    Team2Wins,    // Team 2 has more points and has reached the win threshold
    Draw          // Both have reached the threshold with exactly equal scores
}; 

/**
 * @brief Defines the possible outcomes of handling a player action within a turn.
 */
enum class TurnActionStatus {
    Success_TurnContinues,      // Action valid (draw, take pile, meld), turn proceeds to Meld/Discard phase
    Success_TurnOver,           // Discard valid, turn ended normally
    Success_WentOut,            // Meld or Discard valid, player went out, turn ended
    Error_MainDeckEmpty,        // Attempted to draw from an empty main deck
    Error_MainDeckEmptyDiscardPileCantBeTaken, // Attempted to take discard pile when main deck is empty
    Error_InvalidAction,        // General invalid action (e.g., wrong phase, bad cards, invalid discard type)
    Error_InvalidMeld,          // Invalid meld (e.g., not enough cards, wrong rank)
    Error_MeldRequirementNotMet,// Initial meld points not met after drawing deck (discard rejected, player must retry meld/discard)
};

struct TurnActionResult {
    TurnActionStatus status;
    std::string     message;
};

class RuleEngine {
public:
    RuleEngine() = delete;   // no instances
    ~RuleEngine() = delete;

    // --- Constants ---
    static constexpr int GOING_OUT_BONUS = 100;
    // Define canasta requirements (can be adjusted based on rules)
    static constexpr int MIN_CANASTAS_TO_GO_OUT = 1; // Example: At least one canasta needed

    //static constexpr int WINNING_SCORE = 5000;
    static constexpr int WINNING_SCORE = 3000; // just for testing 

    static constexpr std::size_t STRICT_COMMITMENT_COUNT = 2 + 1; // 2 cards from hand + top card from pile both for initializing or adding to a meld
    static constexpr std::size_t EASY_COMMITMENT_COUNT = 1; // top card from pile for adding to a meld

    static std::expected<int, std::string> validateRankMeldInitializationProposals(
        const std::vector<RankMeldProposal>& proposals);

    static Status validateBlackThreeMeldInitializationProposal(
        const BlackThreeMeldProposal& blackThreeProposal,
        const TeamRoundState& teamRoundState);

    static Status validateRankMeldAdditionProposals(
        const std::vector<RankMeldProposal>& proposals,
        const TeamRoundState& teamRoundState);

    static std::expected<void, int> validatePointsForInitialMelds(
        int initialMeldPoints, int teamTotalScore);

    /// Check whether the player may take the discard-pile, and how:
    /// • On failure: returns an error message.
    /// • On success: returns a DiscardPilePermission indicating the allowed action.
    static std::expected<MeldCommitment, std::string> checkTakingDiscardPile(
                                const Hand& playerHand,
                                const Card& topDiscardCard,
                                const TeamRoundState& teamRoundState,
                                bool isPileFrozen);

    static bool canDiscard(const Hand& playerHand, const Card& discardCard);

    static std::expected<MeldSuggestion, std::string>
    suggestMeld(const std::vector<Card>& cards);

    /// Check if the player can go out before potential discard.
    static bool canGoingOut
    (std::size_t cardsPotentiallyLeftInHandCount, const TeamRoundState& teamRoundState);

    static GameOutcome checkGameOutcome(int team1TotalScore, int team2TotalScore);

    static Status addRedThreeCardsToMeld
    (const std::vector<Card>& redThreeCards, BaseMeld* redThreeMeld);

    template<typename T>
    static std::vector<T> randomRotate(std::vector<T> vec);

private:
    // Helper to get the minimum initial meld points based on score
    static int getMinimumInitialMeldPoints(int teamTotalScore);

    static std::size_t getCanastaCount(const std::vector<std::unique_ptr<BaseMeld>>& teamMelds);

    static int calculateCardPoints(const std::vector<Card>& cards);

    static bool checkIfHandHasCardsWithRank(const Hand& playerHand, Rank rank, std::size_t count = 1);

    /// Create Meld<R>, initialize it with `cards`, and return it.
    /// rank should be from Four to Ace.
    /// On success: a unique_ptr<BaseMeld> holding the initialized meld.
    /// On failure: the exact error string.
    static std::expected<std::unique_ptr<BaseMeld>, std::string>
    createAndInitializeRankMeld(const std::vector<Card>& cards, Rank rank);

    static Status checkCardsAddition(const std::vector<Card>& cards,
        Rank rank, const TeamRoundState& teamRoundState);
};

template<typename T>
std::vector<T> RuleEngine::randomRotate(std::vector<T> vec) {
    if (vec.empty()) return vec;

    static std::mt19937 rng{std::random_device{}()};
    std::uniform_int_distribution<std::size_t> dist(0, vec.size() - 1);
    std::size_t shift = dist(rng);

    std::rotate(vec.begin(),
                vec.begin() + shift,
                vec.end());
    return vec;
}

#endif // RULE_ENGINE_HPP