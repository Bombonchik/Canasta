#include "team_round_state.hpp"
#include <utility> // For std::move
#include <stdexcept> // For potential errors
#include <algorithm> // For std::any_of

// Define constants for meld indices
namespace {
    namespace {
        // Fixed meld‐slot indices
        constexpr std::size_t RED_THREE_MELD_INDEX   = 0;
        constexpr std::size_t BLACK_THREE_MELD_INDEX = 1;
        constexpr std::size_t RANK_MELD_OFFSET       = 2;
    
        // Rank range for normal melds
        constexpr int           FIRST_RANK   = static_cast<int>(Rank::Four);
        constexpr int           LAST_RANK    = static_cast<int>(Rank::Ace);
        constexpr std::size_t   RANK_COUNT   = static_cast<std::size_t>(LAST_RANK - FIRST_RANK + 1);
    
        // Total slots = 2 special melds + one per rank
        constexpr std::size_t TOTAL_MELD_TYPES = RANK_MELD_OFFSET + RANK_COUNT;
    }
}

TeamRoundState::TeamRoundState() {
    createMelds();
}

void TeamRoundState::reset() {
    melds.clear();
    createMelds(); // Recreate melds for a new round
}

// Helper to get index for a given rank (internal use)
std::optional<std::size_t> TeamRoundState::getIndexForRank(Rank r) const {
    int rankInt = static_cast<int>(r);
    if (rankInt >= static_cast<int>(Rank::Four) && rankInt <= static_cast<int>(Rank::Ace)) {
        // Rank::Four (4) maps to index 2, Rank::Ace (14) maps to index 12
        return static_cast<std::size_t>(rankInt - static_cast<int>(Rank::Four) + RANK_MELD_OFFSET);
    }
    return std::nullopt; // Invalid rank for a standard meld
}

void TeamRoundState::createMelds() {
    // Resize vector to hold all melds
    melds.resize(TOTAL_MELD_TYPES);

    // 1) Special three‑card melds
    melds[RED_THREE_MELD_INDEX]   = std::make_unique<RedThreeMeld>();
    melds[BLACK_THREE_MELD_INDEX] = std::make_unique<BlackThreeMeld>();

    // templated (generic) lambda: for each Is in the sequence, create the corresponding Meld<r>
     // 3) Unroll the rank-melds at compile time
    auto instantiateRankMelds = [this]<std::size_t... Is>(std::index_sequence<Is...>) {
        (
            // one expression per Is
            ( melds[RANK_MELD_OFFSET + Is] =
                std::make_unique<
                    Meld< static_cast<Rank>(FIRST_RANK + Is) >
                >()
            ),
        ...);
    };
    instantiateRankMelds(std::make_index_sequence<RANK_COUNT>{});
}

// Get the team's melds (read-only access to the pointers)
const std::vector<std::unique_ptr<BaseMeld>>& TeamRoundState::getMelds() const {
    return melds;
}

// Check if the team has made an initial rank meld
bool TeamRoundState::hasMadeInitialRankMeld() const {
    // Check if any meld is initialized
    auto firstRankMeld = melds.begin() + RANK_MELD_OFFSET;
    return std::any_of(firstRankMeld, melds.end(),
        [](const std::unique_ptr<BaseMeld>& meldPtr) {
            return meldPtr && meldPtr->isInitialized();
        });
}


// Calculate the total points from the team's melds
int TeamRoundState::calculateMeldPoints() const {
    // Use std::accumulate to sum the points from each meld
    return std::accumulate(melds.begin() + 1, melds.end(), 0, // skip red three meld
        [](int sum, const std::unique_ptr<BaseMeld>& meldPtr) {
            // Add the points from the current meld (if the pointer is valid)
            return sum + (meldPtr && meldPtr->isInitialized() ? meldPtr->getPoints() : 0);
        });
}

// Calculate the detailed score breakdown for the round
ScoreBreakdown TeamRoundState::getScoreBreakdown(int goingOutBonus) const {
    ScoreBreakdown breakdown;

    // --- Calculate Meld Points Breakdown ---
    for (size_t i = 0; i < melds.size(); ++i) {
        const auto& meldPtr = melds[i];

        // Skip uninitialized or null melds
        if (!meldPtr || !meldPtr->isInitialized()) {
            continue;
        }

        int meldTotalPoints = meldPtr->getPoints();

        // Handle Red Three Meld separately
        if (i == RED_THREE_MELD_INDEX) {
            // Points from RedThreeMeld are bonus points
            // If the team hasn't made an initial meld, make the points negative
            auto currentMeldRedThreeBonusPoints = meldTotalPoints * (hasMadeInitialRankMeld() ? 1 : -1);
            breakdown.setRedThreeBonusPoints(breakdown.getRedThreeBonusPoints() + currentMeldRedThreeBonusPoints);
            continue; // Skip further processing for this meld
        }

        int canastaBonus = 0;
        if (meldPtr->isCanastaMeld()) {
            std::optional<CanastaType> type = meldPtr->getCanastaType();
            if (type == CanastaType::Natural) {
                canastaBonus = NATURAL_CANASTA_BONUS;
                breakdown.setNaturalCanastaBonus(breakdown.getNaturalCanastaBonus() + canastaBonus);
            } else if (type == CanastaType::Mixed) {
                canastaBonus = MIXED_CANASTA_BONUS;
                breakdown.setMixedCanastaBonus(breakdown.getMixedCanastaBonus() + canastaBonus);
            }
            // else: Should not happen if isCanastaMeld is true, handle error?
        }
        // Card points are the total points minus any canasta bonus
        auto currentMelededCardsPoints = meldTotalPoints - canastaBonus;
        breakdown.setMeldedCardsPoints(breakdown.getMeldedCardsPoints() + currentMelededCardsPoints);
    }

    breakdown.setGoingOutBonus(goingOutBonus);

    return breakdown;
}

// Get a pointer to modify a specific rank-based meld (4-Ace)
BaseMeld* TeamRoundState::getMeldForRank(Rank r) {
    auto indexOpt = getIndexForRank(r);
    if (indexOpt.has_value() && *indexOpt < melds.size() && melds[*indexOpt]) {
        return melds[*indexOpt].get(); // Return raw pointer
    }
    return nullptr; // Rank not valid or meld not initialized correctly
}


// Get a pointer to modify the Red Three meld
BaseMeld* TeamRoundState::getRedThreeMeld() {
    if (RED_THREE_MELD_INDEX < melds.size() && melds[RED_THREE_MELD_INDEX]) {
        return melds[RED_THREE_MELD_INDEX].get();
    }
    return nullptr;
}

BaseMeld* TeamRoundState::getBlackThreeMeld() {
    if (BLACK_THREE_MELD_INDEX < melds.size() && melds[BLACK_THREE_MELD_INDEX]) {
        return melds[BLACK_THREE_MELD_INDEX].get();
    }
    return nullptr;
}

// Const overload (for read-only callers, like RuleEngine)
const BaseMeld* TeamRoundState::getMeldForRank(Rank r) const {
    return const_cast<TeamRoundState*>(this)->getMeldForRank(r);
}

// Const overload for Black Three Meld
const BaseMeld* TeamRoundState::getBlackThreeMeld() const {
    return const_cast<TeamRoundState*>(this)->getBlackThreeMeld();
}

// Const overload for Red Three Meld
const BaseMeld* TeamRoundState::getRedThreeMeld() const {
    return const_cast<TeamRoundState*>(this)->getRedThreeMeld();
}

// Clone the TeamRoundState object
TeamRoundState TeamRoundState::clone() const {
    TeamRoundState clone;
    clone.melds.clear();
    clone.melds.reserve(melds.size());
    for (const auto& meldPtr : melds) {
        if (meldPtr) {
            clone.melds.push_back(meldPtr->clone());
        } else {
            clone.melds.push_back(nullptr);
        }
    }
    return clone;
}

// Note: The Cereal serialization function template is defined inline
// in the header file (team_round_state.hpp) and does not need a separate implementation here.