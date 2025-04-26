#include "server/turn_manager.hpp"
#include "rule_engine.hpp" // Include for static RuleEngine methods
#include <stdexcept> // For potential exceptions if needed
#include <numeric>   // For std::accumulate if calculating points
#include <cassert>

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
    teamHasInitialMeld(teamAlreadyHasInitialMeld),
    teamTotalScore(teamTotalScore),
    drewFromDeck(false),
    tookDiscardPile(false),
    meldsHandled(false),
    mainDeckBecameEmpty(false)
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
        .takeDiscardPile(!teamHasInitialMeld);
    if (!takeDiscardPileResult.has_value()) {
        //provisionalMode = false; // Reset provisional mode
        return {
            TurnActionStatus::Error_InvalidAction,
            takeDiscardPileResult.error()
        };
    }
    hand.get().addCards(takeDiscardPileResult.value(), !teamHasInitialMeld);

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

    auto checkResult = checkMeldRequestsCardsInHand(meldRequests);
    if (!checkResult.has_value())
        return checkResult.error();
    size_t cardsPotentiallyLeftInHandCount = checkResult.value();

    std::vector<std::vector<Card>> meldSuggestions;
    std::vector<RankMeldProposal> rankInitializationProposals, additionProposals;
    std::optional<BlackThreeMeldProposal> blackThreeInitializationProposal;

    processMeldRequests(meldRequests, meldSuggestions, additionProposals);
    
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
    auto rankAdditionStatus = processRankAdditionProposals(additionProposals);
    if (!rankAdditionStatus.has_value())
        return rankAdditionStatus.error();

    // Check if the black three initialization proposal is valid
    auto blackThreeInitializationStatus = processBlackThreeInitializationProposal(
        blackThreeInitializationProposal, cardsPotentiallyLeftInHandCount);
    if (!blackThreeInitializationStatus.has_value())
        return blackThreeInitializationStatus.error();

    initializeRankMelds(rankInitializationProposals);
    addCardsToExistingMelds(additionProposals);
    initializeBlackThreeMeld(blackThreeInitializationProposal);
    
    meldsHandled = true; // Mark that melds have been handled
    

    return {
        TurnActionStatus::Success_TurnContinues,
        "Melds processed successfully."
    };
}

// ToDo: Implement revert take discard pile action

TurnActionResult TurnManager::handleDiscard(const Card& cardToDiscard) {
    if (turnPhase != TurnPhase::MeldDiscard) {
        return TurnActionResult::Error_InvalidAction; // Cannot discard outside of MeldDiscard phase
    }
    // Check if hand actually contains the card
    if (!hand.containsCard(cardToDiscard)) {
         return TurnActionResult::Error_InvalidAction; // Card not in hand
    }
    // Basic rule check (e.g., cannot discard Red Three)
    if (!RuleEngine::canDiscard(hand, cardToDiscard)) { // Assuming RuleEngine has this static check
         return TurnActionResult::Error_InvalidAction; // Invalid card type to discard
    }

    // TODO: Implement actual discard logic
    // 1. If team_has_initial_meld_:
    //    a. Commit: Remove card from hand_, add to discard_pile_
    //    b. Check for going out condition
    //    c. Return Success_TurnOver or Success_WentOut
    // 2. If !team_has_initial_meld_:
    //    a. Perform Final Initial Meld Check on provisional_initial_melds_ using RuleEngine
    //    b. If Check Passes:
    //       i. Commit: commitProvisionalMelds(), remove card from hand, add to discard_pile_
    //       ii. Clear provisional state
    //       iii. Check for going out condition (usually not possible on initial meld)
    //       iv. Return Success_TurnOver
    //    c. If Check Fails:
    //       i. If took_discard_pile_this_turn_:
    //          - revertTakeDiscardPileAction()
    //          - Return Error_RevertAndForceDraw
    //       ii. Else (drew from deck):
    //          - undoProvisionalMeldsAfterDraw()
    //          - Return Error_MeldRequirementNotMet

    // Placeholder implementation
    hand.removeCard(cardToDiscard); // Assume valid for now
    discardPile.push_back(cardToDiscard); // Assume valid for now
    clearProvisionalState(); // Clear any provisional state if it existed

    if (hand.isEmpty() && checkGoingOutCondition()) {
        return TurnActionResult::Success_WentOut;
    } else {
        return TurnActionResult::Success_TurnOver;
    }
}


// --- Private Helper Methods ---

std::expected<std::size_t, TurnActionResult> TurnManager::checkMeldRequestsCardsInHand
(const std::vector<MeldRequest>& meldRequests) const {
    if (meldRequests.empty())
        return std::unexpected(TurnActionResult{
            TurnActionStatus::Error_InvalidMeld,
            "No meld request provided."
        });
    auto cardsInHand = std::vector<Card>(hand.get().getCards());
    for (const auto& request : meldRequests) {
        for (const auto& card : request.cards) {
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

void TurnManager::processMeldRequests
(
    const std::vector<MeldRequest>& meldRequests,
    std::vector<std::vector<Card>>& meldSuggestions,
    std::vector<RankMeldProposal>& additionProposals
) {
    for (const auto& request : meldRequests) {
        if (request.addToRank.has_value()) {
            additionProposals.push_back(RankMeldProposal{
                request.cards,
                request.addToRank.value()
            });
        } else {
            meldSuggestions.push_back(request.cards);
        }
    }
}

std::expected<void, TurnActionResult> TurnManager::processMeldSuggestions
(
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
        auto possibleRank = meld.rank;
        if (meld.type == CandidateMeldType::BlackThree) {
            if (!teamHasInitialMeld)
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
    if (rankInitializationProposals.empty() && !teamHasInitialMeld)
        return std::unexpected(TurnActionResult{
            TurnActionStatus::Error_InvalidMeld,
            "You must initialize at least one meld."
        });
    auto maybePoints = RuleEngine::validateRankMeldInitializationProposals(
        rankInitializationProposals
    );
    if (!maybePoints.has_value())
        return std::unexpected(TurnActionResult{
            TurnActionStatus::Error_InvalidMeld,
            maybePoints.error()
        });

    if (!teamHasInitialMeld) {
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
    if (commitment.has_value() && commitment.value().type == MeldCommitmentType::Initialize)
        return checkInitializationCommitment(rankInitializationProposals, commitment.value());
    return {}; // Return success
}

std::expected<void, TurnActionResult> TurnManager::processBlackThreeInitializationProposal
(const std::optional<BlackThreeMeldProposal>& blackThreeProposal,
    std::size_t cardsPotentiallyLeftInHandCount) const {
    if (!blackThreeProposal.has_value())
        return {}; // No Black Three proposal to process
    auto status = RuleEngine::validateBlackThreeMeldInitializationProposal(
        blackThreeProposal.value(),
        teamRoundState.get(),
        cardsPotentiallyLeftInHandCount
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
    if (rankAdditionProposals.empty())
        return {}; // No rank addition proposals to process
    if (!teamHasInitialMeld)
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
    if (commitment.has_value() && commitment.value().type == MeldCommitmentType::AddToExisting)
        return checkAddToExistingCommitment(rankAdditionProposals, commitment.value());
    return {}; // Return success
}

Status TurnManager::addRedThreeCardsToMeld(const std::vector<Card>& redThreeCards) {
    auto RedThreeMeld = teamRoundState.get().getRedThreeMeld();
    if (RedThreeMeld->isInitialized()) {
        auto status = RedThreeMeld->checkCardsAddition(redThreeCards);
        if (!status.has_value())
            return std::unexpected(status.error());
        RedThreeMeld->addCards(redThreeCards);
        return {};
    }
    auto status = RedThreeMeld->checkInitialization(redThreeCards);
    if (!status.has_value())
        return std::unexpected(status.error());
    RedThreeMeld->initialize(redThreeCards);
}

std::expected<Card, TurnActionResult> TurnManager::drawUntilNonRedThree(ServerDeck& deck) {
    std::vector<Card> redThreeCards;
    while (true)
    {
        auto maybeCard = deck.drawCard();
        if (!maybeCard.has_value()) {
            mainDeckBecameEmpty = true;
            return std::unexpected(TurnActionResult{
                TurnActionStatus::Error_MainDeckEmpty,
                "Main deck is empty. Try taking the discard pile."
            });
        }

        Card card = maybeCard.value();
        if (card.getType() != CardType::RedThree){ // First non-Red Three card
            if (!redThreeCards.empty()) {
                auto status = addRedThreeCardsToMeld(redThreeCards);
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

void TurnManager::clearProvisionalState() {
    provisionalInitialMelds.clear();
    preTakePileHandState.reset();
    // took_discard_pile_this_turn_ should be reset at start of turn or after commit/revert
}

std::expected<void, TurnActionResult> TurnManager::checkInitializationCommitment
(const std::vector<RankMeldProposal>& initializationProposals,
const MeldCommitment& initializationCommitment) const {
    auto commitmentProposal = std::find_if(
        initializationProposals.begin(),
        initializationProposals.end(),
        [&initializationCommitment](const RankMeldProposal& proposal) {
            return proposal.rank == initializationCommitment.rank;
        }
    );
    if (commitmentProposal == initializationProposals.end())
        return std::unexpected(TurnActionResult{
            TurnActionStatus::Error_InvalidMeld,
            "Meld with rank " + to_string(initializationCommitment.rank) + " not found."
        });
    if (commitmentProposal->cards.size() < initializationCommitment.count)
        return std::unexpected(TurnActionResult{
            TurnActionStatus::Error_InvalidMeld,
            "Meld with rank " + to_string(initializationCommitment.rank) +
            " must contain at least " + std::to_string(initializationCommitment.count) +
            " cards."
        });
}

std::expected<void, TurnActionResult> TurnManager::checkAddToExistingCommitment
(const std::vector<RankMeldProposal>& additionProposals,
const MeldCommitment& addToExistingCommitment) const {
    auto commitmentProposal = std::find_if(
        additionProposals.begin(),
        additionProposals.end(),
        [&addToExistingCommitment](const RankMeldProposal& proposal) {
            return proposal.rank == addToExistingCommitment.rank;
        }
    );
    if (commitmentProposal == additionProposals.end())
        return std::unexpected(TurnActionResult{
            TurnActionStatus::Error_InvalidMeld,
            "Card with rank " + to_string(addToExistingCommitment.rank) + 
            " was not added to the existing meld."
        });
    if (commitmentProposal->cards.size() < addToExistingCommitment.count)
        return std::unexpected(TurnActionResult{
            TurnActionStatus::Error_InvalidMeld,
            "You should add to meld with rank " + to_string(addToExistingCommitment.rank) +
            " at least " + std::to_string(addToExistingCommitment.count) +
            " cards."
        });
}

void TurnManager::initializeRankMelds(
    const std::vector<RankMeldProposal>& rankInitializationProposals) {
    for (const auto& proposal : rankInitializationProposals) {
        auto meld = teamRoundState.get().getMeldForRank(proposal.rank);
        assert(meld && "Invariant violated: meld should never be nullopt here");
        auto status = meld->checkInitialization(proposal.cards);
        if (!status.has_value()) // Should never happen
            throw std::runtime_error(status.error());
        meld->initialize(proposal.cards);
    }
}

void TurnManager::addCardsToExistingMelds(
    const std::vector<RankMeldProposal>& additionProposals) {
    for (const auto& proposal : additionProposals) {
        auto meld = teamRoundState.get().getMeldForRank(proposal.rank);
        assert(meld && "Invariant violated: meld should never be nullopt here");
        auto status = meld->checkCardsAddition(proposal.cards);
        if (!status.has_value()) // Should never happen
            throw std::runtime_error(status.error());
        meld->addCards(proposal.cards);
    }
}

void TurnManager::initializeBlackThreeMeld(
    const std::optional<BlackThreeMeldProposal>& blackThreeProposal) {
    if (!blackThreeProposal.has_value())
        return; // No Black Three proposal to initialize
    auto meld = teamRoundState.get().getBlackThreeMeld();
    assert(meld && "Invariant violated: meld should never be nullopt here");
    auto status = meld->checkInitialization(blackThreeProposal->cards);
    if (!status.has_value()) // Should never happen
        throw std::runtime_error(status.error());
    meld->initialize(blackThreeProposal->cards);
}

void TurnManager::revertTakeDiscardPileAction() {
    assert(tookDiscardPile && !teamHasInitialMeld && 
        "Invariant violated: tookDiscardPile should be true and teamHasInitialMeld should be false here");

    hand.get().revertAddCards(); // Revert the hand state
    serverDeck.get().revertTakeDiscardPile(); // Revert the discard pile action

    tookDiscardPile = false;
}

bool TurnManager::checkGoingOutCondition() const {
    // TODO: Implement going out check
    // 1. Check if hand is empty
    // 2. Use RuleEngine to check if team meets requirements (e.g., canasta count)
    if (!hand.get().isEmpty()) {
        return false;
    }
    // Example: Assuming RuleEngine has a static check
    // return RuleEngine::canGoOut(team);
    return true; // Placeholder
}