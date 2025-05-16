#include "server/turn_manager.hpp"
#include "rule_engine.hpp" // Include for static RuleEngine methods
#include <stdexcept> // For potential exceptions if needed
#include <numeric>   // For std::accumulate if calculating points
#include <cassert>
#include "spdlog/spdlog.h" // For logging

// --- Constructor ---

TurnManager::TurnManager(
    Player& player,
    TeamRoundState& teamRoundState,
    ServerDeck& serverDeck,
    bool teamAlreadyHasInitialMeld,
    int teamTotalScore
) : player(std::ref(player)),
    hand(std::ref(player.getHand())),
    teamRoundState(std::ref(teamRoundState)),
    serverDeck(std::ref(serverDeck)),
    teamHasInitialRankMeld(teamAlreadyHasInitialMeld),
    teamTotalScore(teamTotalScore),
    drewFromDeck(false),
    tookDiscardPile(false),
    meldsHandled(false)
{}

// --- Player Action Handlers ---

TurnActionResult TurnManager::handleDrawDeck() {
    if (drewFromDeck)
        return {
            TurnActionStatus::Error_InvalidAction,
            "You have already drawn from the deck."
        }; 

    if (tookDiscardPile)
        return {
            TurnActionStatus::Error_InvalidAction,
            "You cannot draw from the deck after taking the discard pile."
        };

    auto maybeDrawnCard = drawUntilNonRedThree(serverDeck.get());
    if (!maybeDrawnCard.has_value())
        return maybeDrawnCard.error();

    hand.get().addCard(maybeDrawnCard.value());
    drewFromDeck = true; // Mark that the player has drawn from the deck

    return {
        TurnActionStatus::Success_TurnContinues,
        "Card drawn successfully."
    };
}

TurnActionResult TurnManager::handleTakeDiscardPile() {
    if (drewFromDeck)
        return {
            TurnActionStatus::Error_InvalidAction,
            "You cannot take the discard pile after drawing from the deck."
        };
    if (tookDiscardPile)
        return {
            TurnActionStatus::Error_InvalidAction,
            "You have already taken the discard pile."
        };

    auto topDiscard = serverDeck.get().getTopDiscard();
    assert(topDiscard.has_value() && "Invariant violated: top discard should never be nullopt");
    Card topDiscardCard = topDiscard.value();

    auto checkTakingDiscardPileResult = RuleEngine::checkTakingDiscardPile(
        hand.get(),
        topDiscardCard,
        teamRoundState.get(),
        serverDeck.get().isFrozen()
    );
    if (!checkTakingDiscardPileResult.has_value()) {
        return {
            TurnActionStatus::Error_InvalidAction,
            checkTakingDiscardPileResult.error()
        };
    }
    

    auto takeDiscardPileResult = serverDeck.get()
        .takeDiscardPile(true);
        //.takeDiscardPile(!teamHasInitialRankMeld);
    if (!takeDiscardPileResult.has_value()) {
        return {
            TurnActionStatus::Error_InvalidAction,
            takeDiscardPileResult.error()
        };
    }
    //hand.get().addCards(takeDiscardPileResult.value(), !teamHasInitialRankMeld);
    hand.get().addCards(takeDiscardPileResult.value(), true);

    spdlog::debug("Setting meld commitment to meld commitment: rank {} cound {} type {}",
        to_string(checkTakingDiscardPileResult.value().getRank()),
        checkTakingDiscardPileResult.value().getCount(),
        static_cast<int>(checkTakingDiscardPileResult.value().getType()));
    commitment = checkTakingDiscardPileResult.value();

    tookDiscardPile = true; // Assume success for now
    return {
        TurnActionStatus::Success_TurnContinues,
        "Discard pile taken successfully."
    };
}

TurnActionResult TurnManager::handleMelds(const std::vector<MeldRequest>& meldRequests) {
    if (!drewFromDeck && !tookDiscardPile)
        return {
            TurnActionStatus::Error_InvalidAction,
            "You must draw from the deck or take the discard pile before melding."
        };
    if (meldsHandled)
        return {
            TurnActionStatus::Error_InvalidAction,
            "You have already handled melds this turn."
        };

    auto checkResult = checkMeldRequestsCardsInHand(meldRequests);
    if (!checkResult.has_value())
        return checkResult.error();
    std::size_t cardsPotentiallyLeftInHandCount = checkResult.value();

    std::vector<std::vector<Card>> meldSuggestions;
    std::optional<BlackThreeMeldProposal> blackThreeInitializationProposal;
    clearProposals(); // Clear any previous proposals

    processMeldRequests(meldRequests, meldSuggestions, rankAdditionProposals);
    
    // Process meld suggestions
    auto meldSuggestionsStatus = processMeldSuggestions(meldSuggestions,
        rankInitializationProposals, blackThreeInitializationProposal);
    if (!meldSuggestionsStatus.has_value())
        return meldSuggestionsStatus.error();

    // Check if the rank initialization proposals are valid
    auto rankInitializationStatus = processRankInitializationProposals(rankInitializationProposals);
    if (!rankInitializationStatus.has_value())
        return rankInitializationStatus.error();

    // Check if the rank addition proposals are valid
    auto rankAdditionStatus = processRankAdditionProposals(rankAdditionProposals);
    if (!rankAdditionStatus.has_value())
        return rankAdditionStatus.error();

    initializeRankMelds(rankInitializationProposals);
    addCardsToExistingMelds(rankAdditionProposals);

    auto canGoingOut = RuleEngine::canGoingOut(
        cardsPotentiallyLeftInHandCount, teamRoundState.get());

    if (!canGoingOut && cardsPotentiallyLeftInHandCount == 0) { // wants to go out but can't
        revertRankMeldActions(rankInitializationProposals, rankAdditionProposals);
        return {
            TurnActionStatus::Error_InvalidAction,
            "You cannot go out."
        };
    }

    // Check if the black three initialization proposal is valid
    auto blackThreeInitializationStatus = processBlackThreeInitializationProposal(
        blackThreeInitializationProposal, canGoingOut);
    if (!blackThreeInitializationStatus.has_value()) {
        // Revert rank initialization and addition proposals in order to maintain consistency
        revertRankMeldActions(rankInitializationProposals, rankAdditionProposals);
        return blackThreeInitializationStatus.error();
    }

    initializeBlackThreeMeld(blackThreeInitializationProposal);
    
    meldsHandled = true; // Mark that melds have been handled

    if (canGoingOut && cardsPotentiallyLeftInHandCount == 0) // immediate going out
        return {
            TurnActionStatus::Success_WentOut,
            "Player has gone out successfully."
        };
    
    return {
        TurnActionStatus::Success_TurnContinues,
        "Melds processed successfully."
    };
}

// ToDo: Implement revert take discard pile action

TurnActionResult TurnManager::handleDiscard(const Card& cardToDiscard) {
    if (!drewFromDeck && !tookDiscardPile)
        return {
            TurnActionStatus::Error_InvalidAction,
            "You must draw from the deck or take the discard pile before discarding."
        };
    assert(!hand.get().isEmpty() && "Invariant violated: hand should not be empty");
    if (!RuleEngine::canDiscard(hand.get(), cardToDiscard))
        return {
            TurnActionStatus::Error_InvalidAction,
            "You cannot discard a card that is not in your hand."
        };
    if (tookDiscardPile && !meldsHandled)
        return {
            TurnActionStatus::Error_InvalidAction,
            "You must handle melds before discarding."
        };

    auto handCardCount = hand.get().cardCount();
    bool canGoingOut = false;
    if (handCardCount == 1) {
        canGoingOut = RuleEngine::canGoingOut(handCardCount, teamRoundState.get());
        if (!canGoingOut)
            return {
                TurnActionStatus::Error_InvalidAction,
                "You don't meet the requirements to go out."
            };
    }
    assert(hand.get().removeCard(cardToDiscard) && "Invariant violated: card should be in hand");
    serverDeck.get().discardCard(cardToDiscard);
    if (canGoingOut)
        return {
            TurnActionStatus::Success_WentOut,
            "Player has gone out successfully."
        };
    
    return {
        TurnActionStatus::Success_TurnOver,
        "Turn over successfully."
    };
}

TurnActionResult TurnManager::handleRevert() {
    if (!tookDiscardPile && !meldsHandled)
        return {
            TurnActionStatus::Error_InvalidAction,
            "You can only revert after taking the discard pile or handling melds."
        };
    spdlog::debug("Reverting turn for player: {}", player.get().getName());
    if (meldsHandled) {
        spdlog::debug("Reverting melds action");
        revertRankMeldActions(rankInitializationProposals, rankAdditionProposals);
    }
    if (tookDiscardPile) {
        spdlog::debug("Reverting discard pile action");
        revertTakeDiscardPileAction();
    }
    return {
        TurnActionStatus::Success_TurnContinues,
        "Turn reverted successfully."
    };
}

// --- Private Helper Methods ---

std::expected<std::size_t, TurnActionResult> TurnManager::checkMeldRequestsCardsInHand
(const std::vector<MeldRequest>& meldRequests) const {
    if (meldRequests.empty())
        return std::unexpected(TurnActionResult{
            TurnActionStatus::Error_InvalidMeld,
            "No meld request provided."
        });
    auto cardsInHand = std::deque<Card>(hand.get().getCards());
    for (const auto& request : meldRequests) {
        auto& requestCards = request.getCards();
        for (const auto& card : requestCards) {
            auto it = std::find(cardsInHand.begin(), cardsInHand.end(), card);
            if (it == cardsInHand.end()) {
                return std::unexpected(TurnActionResult{
                    TurnActionStatus::Error_InvalidMeld,
                    "Wrong meld request: card " + card.toString() + " not in hand"
                });
            } else {
                cardsInHand.erase(it); // Remove the card from the temporary vector
            }
        }
    }
    return cardsInHand.size(); // Return the number of cards left in hand
}

void TurnManager::processMeldRequests(
    const std::vector<MeldRequest>& meldRequests,
    std::vector<std::vector<Card>>& meldSuggestions,
    std::vector<RankMeldProposal>& additionProposals
) {
    for (const auto& request : meldRequests) {
        auto rank = request.getRank();
        auto cards = request.getCards();
        spdlog::debug("Processing meld request of rank = {}",
            rank.has_value() ? to_string(rank.value()) : "None");
        for (const auto& card : cards) {
            spdlog::debug("    Card: {}", card.toString());
        }
        if (rank.has_value()) {
            additionProposals.push_back(RankMeldProposal{
                cards,
                rank.value()
            });
        } else {
            meldSuggestions.push_back(cards);
        }
    }
}

std::expected<void, TurnActionResult> TurnManager::processMeldSuggestions(
    const std::vector<std::vector<Card>>& meldSuggestions, 
    std::vector<RankMeldProposal>& rankInitializationProposals,
    std::optional<BlackThreeMeldProposal>& blackThreeInitializationProposal
) {
    for (const auto& meldSuggestion : meldSuggestions) {
        auto suggestedMeld = RuleEngine::suggestMeld(meldSuggestion);
        if (!suggestedMeld.has_value())
            return std::unexpected(TurnActionResult{
                TurnActionStatus::Error_InvalidMeld,
                suggestedMeld.error()
            });
        auto meld = suggestedMeld.value();
        auto possibleRank = meld.getRank();
        if (meld.getType() == CandidateMeldType::BlackThree) {
            if (!teamHasInitialRankMeld)
                return std::unexpected(TurnActionResult{
                    TurnActionStatus::Error_InvalidMeld,
                    "Cannot form any meld containing Black Three cards before round's minimum point threshold was reached."
                });
            else if (blackThreeInitializationProposal.has_value())
                return std::unexpected(TurnActionResult{
                    TurnActionStatus::Error_InvalidMeld,
                    "Cannot form more than one Black Three meld."
                });
            blackThreeInitializationProposal = BlackThreeMeldProposal{
                meldSuggestion
            };
        } else {
            assert(possibleRank.has_value() && "Invariant violated: possibleRank should never be nullopt here");
            rankInitializationProposals.push_back(RankMeldProposal{
                meldSuggestion,
                possibleRank.value()
            });
        }
    }
    return {}; // Return success
}

std::expected<void, TurnActionResult> TurnManager::processRankInitializationProposals
(const std::vector<RankMeldProposal>& rankInitializationProposals) const {
    if (rankInitializationProposals.empty() && !teamHasInitialRankMeld)
        return std::unexpected(TurnActionResult{
            TurnActionStatus::Error_InvalidMeld,
            "You must initialize at least one meld."
        });
    for (std::size_t i = 0; i < rankInitializationProposals.size(); ++i) {
        auto& proposal = rankInitializationProposals[i];
        auto& proposalCards = proposal.getCards();
        spdlog::debug("Processing meld proposal {}: cards = {}",
            i + 1, proposalCards.size());
        for (const auto& card : proposalCards) {
            spdlog::debug("Card {}: {}", i + 1, card.toString());
        }
    }
    auto maybePoints = RuleEngine::validateRankMeldInitializationProposals(
        rankInitializationProposals
    );
    if (!maybePoints.has_value())
        return std::unexpected(TurnActionResult{
            TurnActionStatus::Error_InvalidMeld,
            maybePoints.error()
        });
    spdlog::debug("Rank initialization proposals points: {}", maybePoints.value());
    if (!teamHasInitialRankMeld) {
        auto validateStatus = RuleEngine::validatePointsForInitialMelds(
            maybePoints.value(), teamTotalScore);

        if (!validateStatus.has_value())
            return std::unexpected(TurnActionResult{
                TurnActionStatus::Error_MeldRequirementNotMet,
                "Your initial melds must have not less than " +
                std::to_string(validateStatus.error()) +
                " points."
            });
    }
    if (commitment.has_value() && commitment.value().getType() == MeldCommitmentType::Initialize)
        return checkInitializationCommitment(rankInitializationProposals, commitment.value());
    return {}; // Return success
}

std::expected<void, TurnActionResult> TurnManager::processBlackThreeInitializationProposal
(const std::optional<BlackThreeMeldProposal>& blackThreeProposal,
    bool canGoingOut) const {
    if (!blackThreeProposal.has_value())
        return {}; // No Black Three proposal to process
    if (!canGoingOut)
        return std::unexpected(TurnActionResult{
            TurnActionStatus::Error_InvalidMeld,
            "You cannot initialize a Black Three meld without going out."
        });
    auto status = RuleEngine::validateBlackThreeMeldInitializationProposal(
        blackThreeProposal.value(),
        teamRoundState.get()
    );
    if (!status.has_value())
        return std::unexpected(TurnActionResult{
            TurnActionStatus::Error_InvalidMeld,
            status.error()
        });
    return {}; // Return success
}

std::expected<void, TurnActionResult> TurnManager::processRankAdditionProposals
(const std::vector<RankMeldProposal>& rankAdditionProposals) const {
    if (!teamHasInitialRankMeld && !rankAdditionProposals.empty())
        return std::unexpected(TurnActionResult{
            TurnActionStatus::Error_InvalidAction,
            "You cannot add to a meld without initial melds."
        });
    auto additionStatus = RuleEngine::validateRankMeldAdditionProposals(
        rankAdditionProposals,
        teamRoundState.get()
    );
    if (!additionStatus.has_value())
        return std::unexpected(TurnActionResult{
            TurnActionStatus::Error_InvalidMeld,
            additionStatus.error()
        });
    if (commitment.has_value() && commitment.value().getType() == MeldCommitmentType::AddToExisting)
        return checkAddToExistingCommitment(rankAdditionProposals, commitment.value());
    return {}; // Return success
}


std::expected<Card, TurnActionResult> TurnManager::drawUntilNonRedThree(ServerDeck& deck) {
    std::vector<Card> redThreeCards;
    while (true)
    {
        auto maybeCard = deck.drawCard();
        if (!maybeCard.has_value()) {
            return std::unexpected(TurnActionResult{
                TurnActionStatus::Error_MainDeckEmpty,
                "Main deck is empty. Try taking the discard pile."
            });
        }

        Card card = maybeCard.value();
        if (card.getType() != CardType::RedThree){ // First non-Red Three card
            if (!redThreeCards.empty()) {
                auto status = RuleEngine::addRedThreeCardsToMeld(redThreeCards, 
                    teamRoundState.get().getRedThreeMeld());
                if (!status.has_value())
                    return std::unexpected(TurnActionResult{
                        TurnActionStatus::Error_InvalidAction,
                        status.error()
                    });
            }
            return card; // Return the first non-Red Three card
        }
        else
            redThreeCards.push_back(card);
    }
}

std::expected<void, TurnActionResult> TurnManager::checkInitializationCommitment
(const std::vector<RankMeldProposal>& initializationProposals,
const MeldCommitment& initializationCommitment) const {
    auto initializationCommitmentRank = initializationCommitment.getRank();
    auto initializationCommitmentCount = initializationCommitment.getCount();
    auto commitmentProposal = std::find_if(
        initializationProposals.begin(),
        initializationProposals.end(),
        [&](const RankMeldProposal& proposal) {
            return proposal.getRank() == initializationCommitmentRank;
        }
    );
    if (commitmentProposal == initializationProposals.end())
        return std::unexpected(TurnActionResult{
            TurnActionStatus::Error_InvalidMeld,
            "Meld with rank " + to_string(initializationCommitmentRank) + " not found."
        });
    std::size_t commitmentCardCount = 0;
    auto& commitmentProposalCards = commitmentProposal->getCards();
    for (const auto& card : commitmentProposalCards) {
        if (card.getRank() == initializationCommitmentRank)
            commitmentCardCount++;
    }
    if (commitmentCardCount < initializationCommitmentCount)
        return std::unexpected(TurnActionResult{
            TurnActionStatus::Error_InvalidMeld,
            "Meld with rank " + to_string(initializationCommitmentRank) +
            " must contain at least " + std::to_string(initializationCommitmentCount) +
            " cards with rank " + to_string(initializationCommitmentRank) + "."
        });
    return {}; // Return success
}

std::expected<void, TurnActionResult> TurnManager::checkAddToExistingCommitment
(const std::vector<RankMeldProposal>& additionProposals,
const MeldCommitment& addToExistingCommitment) const {
    auto addToExistingCommitmentRank = addToExistingCommitment.getRank();
    auto addToExistingCommitmentCount = addToExistingCommitment.getCount();
    auto commitmentProposal = std::find_if(
        additionProposals.begin(),
        additionProposals.end(),
        [&](const RankMeldProposal& proposal) {
            return proposal.getRank() == addToExistingCommitmentRank;
        }
    );
    if (commitmentProposal == additionProposals.end())
        return std::unexpected(TurnActionResult{
            TurnActionStatus::Error_InvalidMeld,
            "Card with rank " + to_string(addToExistingCommitmentRank) + 
            " was not added to the existing meld."
        });
    std::size_t commitmentCardCount = 0;
    auto& commitmentProposalCards = commitmentProposal->getCards();
    for (const auto& card : commitmentProposalCards) {
        if (card.getRank() == addToExistingCommitmentRank)
            commitmentCardCount++;
    }
    if (commitmentCardCount < addToExistingCommitmentCount)
        return std::unexpected(TurnActionResult{
            TurnActionStatus::Error_InvalidMeld,
            "You should add to meld with rank " + to_string(addToExistingCommitmentRank) +
            " at least " + std::to_string(addToExistingCommitmentCount) +
            " cards with rank " + to_string(addToExistingCommitmentRank) + "."
        });
    return {}; // Return success
}

void TurnManager::initializeRankMelds(
    const std::vector<RankMeldProposal>& rankInitializationProposals) {
    for (const auto& proposal : rankInitializationProposals) {
        auto* meld = teamRoundState.get().getMeldForRank(proposal.getRank());
        assert(meld && "Invariant violated: meld should never be nullopt here");
        auto& proposalCards = proposal.getCards();
        auto status = meld->checkInitialization(proposalCards);
        if (!status.has_value()) // Should never happen
            throw std::runtime_error(status.error());
        meld->initialize(proposalCards);
        for (const auto& card : proposalCards) {
            // Remove the cards from the hand
            assert(hand.get().removeCard(card) && "Card should be in hand");
        }
    }
}

void TurnManager::revertRankMeldsInitialization(
    const std::vector<RankMeldProposal>& rankInitializationProposals) {
    for (const auto& proposal : rankInitializationProposals) {
        auto* meld = teamRoundState.get().getMeldForRank(proposal.getRank());
        assert(meld && meld->isInitialized() && "Invariant violated: meld should always be initialized here");
        meld->reset();
        auto& proposalCards = proposal.getCards();
        // Revert the hand state
        for (const auto& card : proposalCards) {
            hand.get().addCard(card); // Add the cards back to the hand
        }
    }
}

void TurnManager::addCardsToExistingMelds(
    const std::vector<RankMeldProposal>& additionProposals) {
    for (const auto& proposal : additionProposals) {
        auto& proposalCards = proposal.getCards();
        auto* meld = teamRoundState.get().getMeldForRank(proposal.getRank());
        assert(meld && "Invariant violated: meld should never be nullopt here");
        auto status = meld->checkCardsAddition(proposalCards);
        if (!status.has_value()) // Should never happen
            throw std::runtime_error(status.error());
        meld->addCards(proposalCards, /*reversible = */true);
        for (const auto& card : proposalCards) {
            // Remove the cards from the hand
            assert(hand.get().removeCard(card) && "Card should be in hand");
        }
    }
}

void TurnManager::revertRankMeldsAddition(
    const std::vector<RankMeldProposal>& additionProposals) {
    for (const auto& proposal : additionProposals) {
        auto* meld = teamRoundState.get().getMeldForRank(proposal.getRank());
        assert(meld && meld->isInitialized() && "Invariant violated: meld should always be initialized here");
        meld->revertAddCards(); // should work
        auto& proposalCards = proposal.getCards();
        for (const auto& card : proposalCards) {
            hand.get().addCard(card); // Add the cards back to the hand
        }
    }
}


void TurnManager::initializeBlackThreeMeld(
    const std::optional<BlackThreeMeldProposal>& blackThreeProposal) {
    if (!blackThreeProposal.has_value())
        return; // No Black Three proposal to initialize
    auto* meld = teamRoundState.get().getBlackThreeMeld();
    assert(meld && "Invariant violated: meld should never be nullopt here");
    auto blackThreeCards = blackThreeProposal.value().getCards();
    auto status = meld->checkInitialization(blackThreeCards);
    if (!status.has_value()) // Should never happen
        throw std::runtime_error(status.error());
    // If we got here it means the turn and round are ended by this player
    meld->initialize(blackThreeCards);
    for (const auto& card : blackThreeCards) {
        // Remove the cards from the hand
        assert(hand.get().removeCard(card) && "Card should be in hand");
    }
}

// need to call when 
void TurnManager::revertTakeDiscardPileAction() {
    assert(tookDiscardPile && "Invariant violated: tookDiscardPile should be true here");
    
    serverDeck.get().revertTakeDiscardPile(); // Revert the discard pile action
    hand.get().revertAddCards(); // Revert the hand state
    commitment.reset(); // Reset the commitment

    tookDiscardPile = false;
}

void TurnManager::revertRankMeldActions(const std::vector<RankMeldProposal>& rankInitializationProposals,
const std::vector<RankMeldProposal>& additionProposals) {

    revertRankMeldsInitialization(rankInitializationProposals);
    revertRankMeldsAddition(additionProposals);
    // Reset the state
    meldsHandled = false;
}

void TurnManager::clearProposals() {
    rankInitializationProposals.clear();
    rankAdditionProposals.clear();
}