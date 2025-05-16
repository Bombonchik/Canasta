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

/**
 * @enum CandidateMeldType
 * @brief Enum representing the type of candidate meld.
 */
enum class CandidateMeldType {
    BlackThree, ///< Black Three meld
    RankMeld    ///< Normal rank meld
};

/**
 * @class MeldSuggestion
 * @brief Class representing a meld suggestion for meld initialization.
 */
class MeldSuggestion {
private:
    CandidateMeldType type;  ///< Type of meld (Black Three or normal rank meld)
    // only set when it is normal Meld
    std::optional<Rank> rank; ///< Rank of the meld (nullopt for Black Three)
public:
    MeldSuggestion(CandidateMeldType type, std::optional<Rank> rank = std::nullopt)
        : type(type), rank(rank) {}

    /**
     * @brief Get the type of meld.
     * @return The type of meld.
     */
    CandidateMeldType getType() const { return type; }

    /**
     * @brief Get the rank of the meld.
     * @return The rank of the meld (nullopt if not applicable).
     */
    std::optional<Rank> getRank() const { return rank; }
};

/**
 * @class RankMeldProposal
 * @brief Class representing a proposal of initialization or addition cards to a meld of a specific rank.
 */
class RankMeldProposal {
private:
    std::vector<Card>    cards;  ///< the exact cards to be melded
    Rank                 rank;   ///< the rank of the meld
public:
    RankMeldProposal(const std::vector<Card>& cards, Rank rank)
        : cards(cards), rank(rank) {}

    /**
     * @brief Get the rank of the meld.
     * @return The rank of the meld.
     */
    Rank getRank() const { return rank; }

    /**
     * @brief Get the cards to be melded.
     * @return A vector of cards to be melded.
     */
    const std::vector<Card>& getCards() const { return cards; }
};

/**
 * @class BlackThreeMeldProposal
 * @brief Class representing a proposal of initialization of a Black Three meld.
 */
class BlackThreeMeldProposal {
private:
    std::vector<Card> cards; ///< the exact cards to be melded
public:
    BlackThreeMeldProposal(const std::vector<Card>& cards)
        : cards(cards) {}

    /**
     * @brief Get the cards to be melded.
     * @return A vector of cards to be melded.
     */
    const std::vector<Card>& getCards() const { return cards; }
};

/**
 * @enum Status
 * @brief Enum representing the type of MeldCommitment.
 */
enum class MeldCommitmentType {
    Initialize,  ///< initialize a new meld
    AddToExisting ///< add to an existing meld
};

/**
 * @class MeldCommitment
 * @brief Class representing a meld commitment.
 * @details This class is used to follow the rules when player takes the discard pile.
 */
class MeldCommitment {
private:
    MeldCommitmentType type;   ///< whether to initialize a new meld or add to an existing one
    Rank               rank;   ///< which rank the player is committing to
    std::size_t       count;   ///< how many cards of that rank should be used during turn
public:
    MeldCommitment(MeldCommitmentType type, Rank rank, std::size_t count)
        : type(type), rank(rank), count(count) {}

    /**
     * @brief Get the type of meld commitment.
     * @return The type of meld commitment.
     */
    MeldCommitmentType getType() const { return type; }

    /**
     * @brief Get the rank of the meld commitment.
     * @return The rank of the meld commitment.
     */
    Rank getRank() const { return rank; }

    /**
     * @brief Get the count of cards in the meld commitment.
     * @return The count of cards in the meld commitment.
     */
    std::size_t getCount() const { return count; }
};

/**
 * @enum GameOutcome
 * @brief Enum representing the possible outcomes after a round.
 */
enum class GameOutcome {
    Continue,     ///< Neither team has reached the threshold → start next round
    Team1Wins,    ///< Team 1 has more points and has reached the win threshold
    Team2Wins,    ///< Team 2 has more points and has reached the win threshold
    Draw          ///< Both have reached the threshold with exactly equal scores
}; 

/**
 * @enum TurnActionStatus
 * @brief Enum representing the status of a player's action during their turn.
 */
enum class TurnActionStatus {
    Success_TurnContinues,      ///< Action valid (draw, take pile, meld); turn proceeds to Meld/Discard phase
    Success_TurnOver,           ///< Discard valid, turn ended normally
    Success_WentOut,            ///< Meld or Discard valid, player went out, turn ended
    Error_MainDeckEmptyDiscardPileCantBeTaken, ///< Failed to take discard pile when main deck is empty
    Error_MainDeckEmpty,        ///< Attempted to draw from an empty main deck
    Error_InvalidAction,        ///< General invalid action (e.g., wrong phase, bad cards, invalid discard type)
    Error_InvalidMeld,          ///< Invalid meld (e.g., not enough cards, wrong rank)
    Error_MeldRequirementNotMet,///< Initial meld points not met
};

/**
 * @class TurnActionResult
 * @brief Class representing the result of a player's action during their turn.
 */
class TurnActionResult {
private:
    TurnActionStatus status; ///< Status of the action
    std::string     message; ///< Message describing the result
public:
    TurnActionResult(TurnActionStatus status, const std::string& message)
        : status(status), message(message) {}

    /**
     * @brief Get the status of the action.
     * @return The status of the action.
     */
    TurnActionStatus getStatus() const { return status; }

    /**
     * @brief Get the message describing the result.
     * @return The message describing the result.
     */
    const std::string& getMessage() const { return message; }
};

/**
 * @class RuleEngine
 * @brief Class containing the rules and validation logic for the game.
 */
class RuleEngine {
public:
    RuleEngine() = delete;   ///< no instances of RuleEngine
    ~RuleEngine() = delete;  ///< no instances of RuleEngine

    // --- Constants ---
    static constexpr int GOING_OUT_BONUS = 100; ///< Bonus points for going out
    static constexpr int MIN_CANASTAS_TO_GO_OUT = 1; ///< Example: At least one canasta needed to go out

    //static constexpr int WINNING_SCORE = 5000;
    // just for testing
    static constexpr int WINNING_SCORE = 3000; ///< Minimum score to win the game

    static constexpr std::size_t STRICT_COMMITMENT_COUNT = 2 + 1; ///< 2 cards from hand + top card from pile both for initializing or adding to a meld
    static constexpr std::size_t EASY_COMMITMENT_COUNT = 1; ///< top card from pile for adding to a meld

    /**
     * @brief Validates the initialization proposals for rank melds.
     * @returns the total points of the initialized melds if valid, or an error message.
     */
    static std::expected<int, std::string> validateRankMeldInitializationProposals(
        const std::vector<RankMeldProposal>& proposals);

    /**
     * @brief Validates the initialization proposal for a Black Three meld.
     */
    static Status validateBlackThreeMeldInitializationProposal(
        const BlackThreeMeldProposal& blackThreeProposal,
        const TeamRoundState& teamRoundState);

    /**
     * @brief Validates the addition proposals for rank melds.
     */
    static Status validateRankMeldAdditionProposals(
        const std::vector<RankMeldProposal>& proposals,
        const TeamRoundState& teamRoundState);

    /**
     * @brief Validates the initial meld points based on the team's total score.
     * @details Checks if it is enough of initial meld points to make the initial melding.
     */
    static std::expected<void, int> validatePointsForInitialMelds(
        int initialMeldPoints, int teamTotalScore);

    /**
     * @brief Checks whether the player may take the discard-pile, and how
     * @details • On failure: returns an error message.
     * @details • On success: returns a MeldCommitment indicating the commitment player should fulfill by the end of the turn.
     */
    static std::expected<MeldCommitment, std::string> checkTakingDiscardPile(
                                const Hand& playerHand,
                                const Card& topDiscardCard,
                                const TeamRoundState& teamRoundState,
                                bool isPileFrozen);

    /**
     * @brief Checks if the player can discard a card.
     */
    static bool canDiscard(const Hand& playerHand, const Card& discardCard);

    /**
     * @brief Suggests a meld based on the provided cards.
     * @details • On failure: returns an error message.
     * @details • On success: returns a MeldSuggestion indicating the type of meld and rank (if applicable).
     */
    static std::expected<MeldSuggestion, std::string>
    suggestMeld(const std::vector<Card>& cards);

    /// Check if the player can go out.
    static bool canGoingOut
    (std::size_t cardsPotentiallyLeftInHandCount, const TeamRoundState& teamRoundState);

    /**
     * @brief Checks the game outcome based on the total scores of both teams.
     */
    static GameOutcome checkGameOutcome(int team1TotalScore, int team2TotalScore);

    /**
     * @brief Initializes RedThree meld or add cards to it if it is initialized.
     * @details On failure: returns an error message.
     */
    static Status addRedThreeCardsToMeld
    (const std::vector<Card>& redThreeCards, BaseMeld* redThreeMeld);

    /**
     * @brief Randomly rotates a vector by a random number of positions.
     */
    template<typename T>
    static std::vector<T> randomRotate(std::vector<T> vec);

private:
    static constexpr int MELD_POINTS_NEGATIVE  =  15;
    static constexpr int MELD_POINTS_LOW       =  50;
    static constexpr int MELD_POINTS_MEDIUM    =  90;
    static constexpr int MELD_POINTS_HIGH      = 120;
    static constexpr int TEAM_SCORE_THRESHOLD_NONE    =    0;
    static constexpr int TEAM_SCORE_THRESHOLD_LOW     = 1500;
    static constexpr int TEAM_SCORE_THRESHOLD_MEDIUM  = 3000;
    // Helper to get the minimum initial meld points based on score
    /**
     * @brief Gets the minimum initial meld points based on the team's total score.
     */
    static int getMinimumInitialMeldPoints(int teamTotalScore);

    /**
     * @brief Gets the number of canastas in the team's melds.
     */
    static std::size_t getCanastaCount(const std::vector<std::unique_ptr<BaseMeld>>& teamMelds);

    /**
     * @brief Calculates the total points for the given cards.
     */
    static int calculateCardPoints(const std::vector<Card>& cards);

    /**
     * @brief Checks if the player's hand has the specified number of cards with the given rank.
     */
    static bool checkIfHandHasCardsWithRank(const Hand& playerHand, Rank rank, std::size_t count = 1);

    /**
     * @brief Creates a Meld<R> and initializes it with the provided cards.
     * @param cards The cards to initialize the meld with.
     * @param rank The rank of the meld (should be from Four to Ace).
     * @returns A unique_ptr to the initialized meld on success, or an error message on failure.
     */
    static std::expected<std::unique_ptr<BaseMeld>, std::string>
    createAndInitializeRankMeld(const std::vector<Card>& cards, Rank rank);

    /**
     * @brief Checks if the cards can be added to the existing meld of the specified rank.
     */
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