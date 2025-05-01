#include "server/round_manager.hpp"
#include "server/turn_manager.hpp"
#include "rule_engine.hpp"
#include "player.hpp"
#include "team_round_state.hpp"
#include "score_details.hpp"
#include "card.hpp"
#include "meld.hpp" // Include for BaseMeld access

#include <stdexcept>
#include <algorithm> // For std::find_if
#include <numeric>   // For std::accumulate
#include "spdlog/spdlog.h"

// --- Constructor ---

RoundManager::RoundManager(
    const std::vector<std::reference_wrapper<Player>>& players,
    std::reference_wrapper<const Team> team1,
    std::reference_wrapper<const Team> team2
) : playersInTurnOrder(players),
    team1(team1), // Store player lists for lookup
    team2(team2),
    team1State(), // Default construct TeamRoundState
    team2State(),
    roundPhase(RoundPhase::NotStarted),
    serverDeck(), // Default constructor initializes and shuffles
    currentPlayerIndex(0), // Starting player index
    playerWhoWentOut(std::nullopt), // Initialize optional member
    isMainDeckEmpty(false) // Initialize deck empty flag
{
    if (playersInTurnOrder.empty()) {
        throw std::invalid_argument("RoundManager requires at least one player.");
    }
}

// --- Public Methods ---

void RoundManager::startRound() {
    if (roundPhase != RoundPhase::NotStarted) {
        spdlog::error("Round already started or finished.");
        return;
    }
    roundPhase = RoundPhase::Dealing;

    // Deck is shuffled in ServerDeck constructor
    dealInitialHands();

    roundPhase = RoundPhase::InProgress;
    setupTurnManagerForCurrentPlayer();
}

Player& RoundManager::getCurrentPlayer() const {
    if (roundPhase != RoundPhase::InProgress) {
        throw std::logic_error("Cannot get current player when round is not in progress.");
    }
    if (currentPlayerIndex >= playersInTurnOrder.size()) {
        throw std::out_of_range("Current player index is out of bounds.");
    }
    return playersInTurnOrder[currentPlayerIndex].get();
}

TurnActionResult RoundManager::handleDrawDeckRequest() {
    if (roundPhase != RoundPhase::InProgress || !currentTurnManager)
        return {
            TurnActionStatus::Error_InvalidAction,
            "Not player's turn or round not in progress."
        };
    if (isMainDeckEmpty)
        return {
            TurnActionStatus::Error_MainDeckEmpty,
            "Main deck is empty. Try taking the discard pile."
        };
    TurnActionResult result = currentTurnManager->handleDrawDeck();
    processTurnResult(result);
    return result;
}

TurnActionResult RoundManager::handleTakeDiscardPileRequest() {
    if (roundPhase != RoundPhase::InProgress || !currentTurnManager) {
        return {TurnActionStatus::Error_InvalidAction, "Not player's turn or round not in progress."};
    }
    TurnActionResult result = currentTurnManager->handleTakeDiscardPile();
    if (result.status != TurnActionStatus::Success_TurnContinues && isMainDeckEmpty)
        result = {
            TurnActionStatus::Error_MainDeckEmptyDiscardPileCantBeTaken,
            "Main deck is empty. Discard pile can't be taken."
        };
    processTurnResult(result);
    return result;
}

TurnActionResult RoundManager::handleMeldRequest(const std::vector<MeldRequest>& meldRequests) {
    if (roundPhase != RoundPhase::InProgress || !currentTurnManager) {
        return {TurnActionStatus::Error_InvalidAction, "Not player's turn or round not in progress."};
    }
    TurnActionResult result = currentTurnManager->handleMelds(meldRequests);
    processTurnResult(result);
    return result;
}

TurnActionResult RoundManager::handleDiscardRequest(const Card& cardToDiscard) {
    if (roundPhase != RoundPhase::InProgress || !currentTurnManager) {
        return {TurnActionStatus::Error_InvalidAction, "Not player's turn or round not in progress."};
    }
    TurnActionResult result = currentTurnManager->handleDiscard(cardToDiscard);
    processTurnResult(result);
    return result;
}

TurnActionResult RoundManager::handleRevertRequest() {
    if (roundPhase != RoundPhase::InProgress || !currentTurnManager) {
        return {TurnActionStatus::Error_InvalidAction, "Not player's turn or round not in progress."};
    }
    TurnActionResult result = currentTurnManager->handleRevert();
    // Revert doesn't end the turn, so no processTurnResult call needed immediately.
    return result;
}


bool RoundManager::isRoundOver() const {
    return roundPhase == RoundPhase::Finished;
}

std::map<std::string, ScoreBreakdown> RoundManager::calculateScores() const {
    if (!isRoundOver()) {
        throw std::logic_error("Cannot calculate scores before round is finished.");
    }

    std::map<std::string, ScoreBreakdown> roundScores;

    std::string team1Name = team1.get().getName();
    std::string team2Name = team2.get().getName();

    int goingOutBonusAmount = 0;
    // Assuming TeamRoundState doesn't store name, use generic names or get from Team if linked

    std::string winningTeamName;
    if (playerWhoWentOut.has_value()) {
        goingOutBonusAmount = RuleEngine::GOING_OUT_BONUS;
        try {
            winningTeamName = getTeamForPlayer(playerWhoWentOut->get()).getName();
        } catch (const std::logic_error& e) {
            throw std::logic_error("Player who went out not found in any team state: " + std::string(e.what()));
        }
    }

    // Calculate base scores from melds for both teams
    roundScores[team1Name] = team1State.getScoreBreakdown(winningTeamName == team1Name ? goingOutBonusAmount : 0);
    roundScores[team2Name] = team2State.getScoreBreakdown(winningTeamName == team2Name ? goingOutBonusAmount : 0);

    for (auto& playerRef : playersInTurnOrder) {
        auto& player = playerRef.get();
        const auto& teamName = getTeamForPlayer(player).getName();
        roundScores[teamName].handPenaltyPoints -= player.getHand().calculatePenalty();
    }

    return roundScores;
}

// --- Private Helpers ---

void RoundManager::dealInitialHands() {
    spdlog::info("Dealing initial hands to players.");
    for (auto& player : playersInTurnOrder) {
        player.get().resetHand();
        auto & playerTeamState = getTeamStateForPlayer(player.get());
        auto & playerHand = player.get().getHand();
        for (std::size_t i = 0; i < INITIAL_HAND_SIZE; ++i) {
            std::vector<Card> redThreeCards;
            while (true) {
                auto maybeCard = serverDeck.drawCard();
                assert(maybeCard.has_value() && "Deck should not be empty when dealing initial hands");
                Card card = maybeCard.value();
                if (card.getType() != CardType::RedThree){ // First non-Red Three card
                    if (!redThreeCards.empty()) {
                        auto status = RuleEngine::addRedThreeCardsToMeld(redThreeCards, 
                            playerTeamState.getRedThreeMeld());
                        assert(status.has_value() && "Failed to add Red Three cards to meld");
                    }
                    playerHand.addCard(card);
                    break; // Exit loop
                }
                else
                    redThreeCards.push_back(card);
            }
        }
    }
}

void RoundManager::setupTurnManagerForCurrentPlayer() {
    if (roundPhase != RoundPhase::InProgress) return;
    spdlog::debug("Setting up TurnManager for current player: {}", getCurrentPlayer().getName());

    auto & player = getCurrentPlayer();
    auto & teamState = getTeamStateForPlayer(player);
    bool teamHasInitial = teamState.hasMadeInitialRankMeld();
    int teamScore = getTeamForPlayer(player).getTotalScore();

    currentTurnManager = std::make_unique<TurnManager>(
        player,
        teamState,
        serverDeck,
        teamHasInitial,
        teamScore
    );
}

void RoundManager::processTurnResult(const TurnActionResult& result) {
    // Handle actions that might end the round or turn
    switch (result.status) {
        case TurnActionStatus::Success_TurnOver:
            advanceToNextPlayer();
            if (!isRoundOver()) {
                setupTurnManagerForCurrentPlayer();
            } else {
                currentTurnManager.reset(); // Round ended
            }
            break;

        case TurnActionStatus::Success_WentOut:
            if (roundPhase == RoundPhase::InProgress) { // Ensure we only process 'went out' once
                roundPhase = RoundPhase::Finished; // Player went out
                playerWhoWentOut = getCurrentPlayer(); // Store who went out
                currentTurnManager.reset(); // Turn manager no longer needed
            }
            break;

        case TurnActionStatus::Error_MainDeckEmpty:
            if (roundPhase == RoundPhase::InProgress) {
                isMainDeckEmpty = true;
            }
            break;

        case TurnActionStatus::Error_MainDeckEmptyDiscardPileCantBeTaken:
            if (roundPhase == RoundPhase::InProgress) {
                roundPhase = RoundPhase::Finished;
                currentTurnManager.reset(); // Turn manager no longer needed
            }
            break;

        // For other statuses turn continues, caller handles message
        case TurnActionStatus::Success_TurnContinues:
        case TurnActionStatus::Error_InvalidAction:
        case TurnActionStatus::Error_InvalidMeld:
        case TurnActionStatus::Error_MeldRequirementNotMet:
        default:
            // Do nothing here, turn continues with the same TurnManager
            break;
    }
}

void RoundManager::advanceToNextPlayer() {
    if (playersInTurnOrder.empty()) return;
    currentPlayerIndex = (currentPlayerIndex + 1) % playersInTurnOrder.size();
}

const Team& RoundManager::getTeamForPlayer(const Player& player) const {
    // Check if player is in team 1
    if (team1.get().hasPlayer(player)) {
        return team1.get();
    }

    // Check if player is in team 2
    if (team2.get().hasPlayer(player)) {
        return team2.get();
    }

    throw std::logic_error("Player " + player.getName() + " not found in any team within RoundManager.");
}

TeamRoundState& RoundManager::getTeamStateForPlayer(Player& player) {
    // Check if player is in team 1
    if (team1.get().hasPlayer(player)) {
        return team1State;
    }

    // Check if player is in team 2
    if (team2.get().hasPlayer(player)) {
        return team2State;
    }

    throw std::logic_error("Player " + player.getName() + " not found in any team state within RoundManager.");
}

// Const overload
const TeamRoundState& RoundManager::getTeamStateForPlayer(const Player& player) const {
    return const_cast<RoundManager*>(this)
    ->getTeamStateForPlayer(const_cast<Player&>(player));
}

ClientDeck RoundManager::getClientDeck() const {
    return ClientDeck{
        serverDeck.mainDeckSize(),
        serverDeck.getTopDiscard(),
        serverDeck.discardPileSize(),
        serverDeck.isFrozen()
    };
}

std::vector<PlayerPublicInfo> RoundManager::getAllPlayersPublicInfo() const {
    std::vector<PlayerPublicInfo> playerInfos;
    for (const auto& playerRef : playersInTurnOrder) {
        const Player& player = playerRef.get();
        playerInfos.push_back({
            player.getName(),
            player.getHand().cardCount(),
            player.getName() == getCurrentPlayer().getName()
        });
    }
    return playerInfos;
}

TeamRoundState RoundManager::getTeamStateForTeam(const Team& team) const {
    if (team.getName() == team1.get().getName()) {
        return team1State.clone();
    } else if (team.getName() == team2.get().getName()) {
        return team2State.clone();
    }
    throw std::logic_error("Team " + team.getName() + " not found in RoundManager.");
}