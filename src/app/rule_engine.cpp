#include "rule_engine.hpp"
#include <array>
//#include "spdlog/spdlog.h"

bool RuleEngine::canDiscard(const Hand& playerHand, const Card& discardCard) {
    // Check if the card is in the player's hand
    return playerHand.hasCard(discardCard);
}

    int RuleEngine::getMinimumInitialMeldPoints(int teamTotalScore) {
        // Calculate the minimum initial meld points based on the team's total score
        if (teamTotalScore < TEAM_SCORE_THRESHOLD_NONE) {
            return MELD_POINTS_NEGATIVE;
        } else if (teamTotalScore < TEAM_SCORE_THRESHOLD_LOW) {
            return MELD_POINTS_LOW;
        } else if (teamTotalScore < TEAM_SCORE_THRESHOLD_MEDIUM) {
            return MELD_POINTS_MEDIUM;
        } else {
            return MELD_POINTS_HIGH;
        }
    }

int RuleEngine::calculateCardPoints(const std::vector<Card>& cards) {
    // Calculate the total point value of a list of cards according to Canasta rules
    int totalPoints = 0;
    for (const auto& card : cards) {
        totalPoints += card.getPoints();
    }
    return totalPoints;
}

std::expected<int, std::string> RuleEngine::validateRankMeldInitializationProposals(
    const std::vector<RankMeldProposal>& proposals) {
    int totalPoints = 0;
    for (const auto& proposal : proposals) {
        // Create and initialize the meld for the given rank
        const auto meld = createAndInitializeRankMeld(proposal.getCards(), proposal.getRank());
        if (!meld.has_value())
            return std::unexpected(meld.error()); // Return the error message
        //spdlog::debug("Meld created for rank {}: {} cards, {} points", (*meld)->isInitialized(), (*meld)->getCards().size(), (*meld)->getPoints());
        totalPoints += (*meld)->getPoints();
    }
    return totalPoints; // Success
}

Status RuleEngine::validateBlackThreeMeldInitializationProposal(
const BlackThreeMeldProposal& blackThreeProposal, const TeamRoundState& teamRoundState) {
    const auto blackThreeMeld = teamRoundState.getBlackThreeMeld();
    assert(blackThreeMeld && "Black Three meld is null");
    auto status = blackThreeMeld->checkInitialization(blackThreeProposal.getCards());
    if (!status.has_value())
        return std::unexpected(status.error());
    return {}; // Successss
}

Status RuleEngine::validateRankMeldAdditionProposals(
    const std::vector<RankMeldProposal>& proposals,
    const TeamRoundState& teamRoundState) {
    for (const auto& proposal : proposals) {

        auto status = RuleEngine::checkCardsAddition
        (proposal.getCards(), proposal.getRank(), teamRoundState);
        if (!status.has_value())
            return std::unexpected(status.error());
    }
    return {}; // Success
}

std::expected<void, int> RuleEngine::validatePointsForInitialMelds(
    int initialMeldPoints, int teamTotalScore) {
    int minPoints = getMinimumInitialMeldPoints(teamTotalScore);
    // Check if the proposed meld meets the minimum points requirement
    if (initialMeldPoints < minPoints) {
        return std::unexpected(minPoints); // Return the minimum required points
    }
    return {}; // Success
}

std::expected<MeldSuggestion, std::string> RuleEngine::suggestMeld(const std::vector<Card>& cards) {
    // Check if the cards are valid for a meld
    if (cards.empty()) {
        return std::unexpected("No cards provided for melding");
    }
    for (auto const& card : cards) {
        auto t = card.getType();
        if (t == CardType::RedThree) { // Shouldn't happen
            return std::unexpected{
                "Can not form any meld containing Red Three cards"
            };
        }
        else if (t == CardType::BlackThree) { 
            return MeldSuggestion{
                CandidateMeldType::BlackThree,
                std::nullopt
            };
        }
        else if (t == CardType::Natural) {
            return MeldSuggestion{
                CandidateMeldType::RankMeld,
                card.getRank()
            };
        }
    }
    return std::unexpected{
        "No natural cards present; cannot form a rank-based meld"
    };
}

bool RuleEngine::canGoingOut(std::size_t cardsPotentiallyLeftInHandCount, const TeamRoundState& teamRoundState) {
    // Check if the player can go out
    
    const auto canastaCount = RuleEngine::getCanastaCount(teamRoundState.getMelds());
    return cardsPotentiallyLeftInHandCount <= 1 && canastaCount >= MIN_CANASTAS_TO_GO_OUT;
}

template <Rank R>
static std::expected<std::unique_ptr<BaseMeld>, std::string>
createAndInitializeRankMeldImpl(const std::vector<Card>& cards) {
    auto meldPtr = std::make_unique<Meld<R>>();
    if (auto status = meldPtr->checkInitialization(cards); !status.has_value()) {
        return std::unexpected{ status.error() };
    }
    meldPtr->initialize(cards);
    return meldPtr;
}

std::expected<std::unique_ptr<BaseMeld>, std::string>
RuleEngine::createAndInitializeRankMeld(const std::vector<Card>& cards, Rank rank)
{
    constexpr int FIRST_RANK = static_cast<int>(Rank::Four);
    constexpr int LAST_RANK  = static_cast<int>(Rank::Ace);
    constexpr std::size_t COUNT   = LAST_RANK - FIRST_RANK + 1;

    // Once we find the matching R, we store its success/failure here:
    std::optional<std::expected<std::unique_ptr<BaseMeld>, std::string>>
        initializationResult;

    // Templated lambda: unpacks Is = 0..COUNT-1 at compile time
    auto dispatch = [&]<std::size_t... Is>(std::index_sequence<Is...>) {
        (
            // This fold expands into COUNT runs of the following lambda:
            [&] {
                if (initializationResult) 
                    return;         // already done

                constexpr Rank currentRank =
                    static_cast<Rank>(FIRST_RANK + Is);

                if (rank == currentRank) {
                    initializationResult =
                        createAndInitializeRankMeldImpl<currentRank>(cards);
                }
            }(),
        ...);
    };

    // Invoke the templated lambda with an index_sequence<0,1,…,COUNT-1>
    dispatch(std::make_index_sequence<COUNT>{});

    if (initializationResult.has_value())
        return std::move(*initializationResult);

    return std::unexpected{
        "Rank " + std::to_string(static_cast<int>(rank))
        + " is not a valid normal meld rank"
    };
}

Status RuleEngine::checkCardsAddition(const std::vector<Card>& cards,
    Rank rank, const TeamRoundState& teamRoundState) {
    const BaseMeld* meld = teamRoundState.getMeldForRank(rank);
    if (!meld || !meld->isInitialized())
        return std::unexpected("Meld not initialized for rank " + to_string(rank));

    Status status = meld->checkCardsAddition(cards);
    if (!status.has_value())
        return std::unexpected(status.error());
    return {}; // Success
}

std::size_t RuleEngine::getCanastaCount(const std::vector<std::unique_ptr<BaseMeld>>& teamMelds) {
    // Count the number of canasta melds in the team
    return std::count_if(teamMelds.begin(), teamMelds.end(),
        [](const std::unique_ptr<BaseMeld>& meldPtr) {
            return meldPtr && meldPtr->isInitialized() && meldPtr->isCanastaMeld();
        });
}

bool RuleEngine::checkIfHandHasCardsWithRank(const Hand& playerHand, Rank rank, std::size_t count) {
    // Check if the player's hand has the specified number of cards with the given rank
    const auto& playerCards = playerHand.getCards();
    return std::count_if(playerCards.begin(), playerCards.end(),
        [rank](const Card& card) { return card.getRank() == rank; }) >= count;
}

std::expected<MeldCommitment, std::string> RuleEngine::checkTakingDiscardPile(
                                const Hand& playerHand,
                                const Card& topDiscardCard,
                                const TeamRoundState& teamRoundState,
                                bool isPileFrozen) {
    bool hasMadeInitialMeld = teamRoundState.hasMadeInitialRankMeld();
    Rank topDiscardCardRank = topDiscardCard.getRank();
    bool hasCardsWithRank = checkIfHandHasCardsWithRank(playerHand, topDiscardCardRank, STRICT_COMMITMENT_COUNT - 1);
    const BaseMeld* meld = teamRoundState.getMeldForRank(topDiscardCardRank);
    // Check if the player can take the discard pile and initialize a meld
    if (hasCardsWithRank && (!meld || !meld->isInitialized()))
        return MeldCommitment{
            MeldCommitmentType::Initialize,
            topDiscardCardRank,
            STRICT_COMMITMENT_COUNT
        };
    // If player isn't able to initialize a meld, check if they can add to an existing one
    if (!hasMadeInitialMeld)
        return std::unexpected{
            "Cannot take discard pile: you must have at least one initialized meld"
        };
    
    // Pile is frozen
    if (isPileFrozen) {
        if (hasCardsWithRank && meld && meld->isInitialized()) {
            return MeldCommitment{
                MeldCommitmentType::AddToExisting,
                topDiscardCardRank,
                STRICT_COMMITMENT_COUNT
            };
        }
        return std::unexpected{
            "Cannot take discard pile: it is frozen "
            "and you don't have 2 cards of rank " + to_string(topDiscardCardRank)
        };
    }
    // Pile isn't frozen
    // Check if the player's team has an initialized meld of the same rank
    if (!meld || !meld->isInitialized()) {
        return std::unexpected{
            "Cannot take discard pile: no initialized meld of rank "
            + to_string(topDiscardCardRank)
        };
    }
    if (meld->isCanastaMeld()) {
        return std::unexpected{
            "Cannot take discard pile: the meld of rank " + 
            to_string(topDiscardCardRank) + " is already a canasta"
        };
    }
    // player can take the pile and should add the top card to the meld of that rank
    return MeldCommitment {
        MeldCommitmentType::AddToExisting,
        topDiscardCardRank,
        EASY_COMMITMENT_COUNT
    };
}

GameOutcome RuleEngine::checkGameOutcome(int team1TotalScore, int team2TotalScore) {
    // If neither has hit the target, we continue
    if (team1TotalScore < WINNING_SCORE && team2TotalScore < WINNING_SCORE) {
        return GameOutcome::Continue;
    }

    // If both have reached or exceeded it...
    if (team1TotalScore >= WINNING_SCORE && team2TotalScore >= WINNING_SCORE) {
        if (team1TotalScore > team2TotalScore)   return GameOutcome::Team1Wins;
        else if (team2TotalScore > team1TotalScore) return GameOutcome::Team2Wins;
        else                            return GameOutcome::Draw;
    }

    // Exactly one team has reached it
    if (team1TotalScore >= team2TotalScore) return GameOutcome::Team1Wins;
    else                             return GameOutcome::Team2Wins;
}

Status RuleEngine::addRedThreeCardsToMeld
(const std::vector<Card>& redThreeCards, BaseMeld* redThreeMeld) {
    if (redThreeMeld->isInitialized()) {
        auto status = redThreeMeld->checkCardsAddition(redThreeCards);
        if (!status.has_value())
            return std::unexpected(status.error());
        redThreeMeld->addCards(redThreeCards);
        return {};
    }
    auto status = redThreeMeld->checkInitialization(redThreeCards);
    if (!status.has_value())
        return std::unexpected(status.error());
    redThreeMeld->initialize(redThreeCards);
    return {};
}