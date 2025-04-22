#include "team.hpp"
#include <utility> // For std::move
#include <stdexcept> // For potential errors
#include <algorithm> // For std::any_of

// Define constants for meld indices
namespace {
    const size_t RED_THREE_MELD_INDEX = 0;
    const size_t BLACK_THREE_MELD_INDEX = 1;
    const size_t RANK_MELD_OFFSET = 2; // Start rank melds from index 2
    const size_t TOTAL_MELD_TYPES = 13; // Red3, Black3, 4-Ace (11 ranks)
}

// Constructor - Takes a team name and moves it into the member variable
Team::Team(std::string name) : name(std::move(name)) {
    createMelds();
}

// Default constructor
Team::Team() {
    createMelds();
}

// Helper to get index for a given rank (internal use)
std::optional<size_t> Team::getIndexForRank(Rank r) const {
    int rankInt = static_cast<int>(r);
    if (rankInt >= static_cast<int>(Rank::Four) && rankInt <= static_cast<int>(Rank::Ace)) {
        // Rank::Four (4) maps to index 2, Rank::Ace (14) maps to index 12
        return static_cast<size_t>(rankInt - static_cast<int>(Rank::Four) + RANK_MELD_OFFSET);
    }
    return std::nullopt; // Invalid rank for a standard meld
}

void Team::createMelds() {
    // Resize vector to hold all melds
    melds.clear();
    melds.resize(TOTAL_MELD_TYPES);

    // 1) Special three‑card melds
    melds[RED_THREE_MELD_INDEX]   = std::make_unique<RedThreeMeld>();
    melds[BLACK_THREE_MELD_INDEX] = std::make_unique<BlackThreeMeld>();

    constexpr int first = static_cast<int>(Rank::Four);
    constexpr int last  = static_cast<int>(Rank::Ace);
    constexpr size_t N   = static_cast<size_t>(last - first + 1);

    // templated (generic) lambda: for each Is in the sequence, create the corresponding Meld<r>
    auto instantiateRankMelds = 
        [this, first]<std::size_t... Is>(std::index_sequence<Is...>) {
        // use a fold expression to expand the pack
        ((
            [&] {
                constexpr Rank r = static_cast<Rank>(first + Is);
                auto idxOpt = getIndexForRank(r);
                if (!idxOpt) {
                    throw std::logic_error(
                        "createMelds: cannot determine index for rank " +
                        std::to_string(static_cast<int>(r))
                    );
                }
                melds[*idxOpt] = std::make_unique<Meld<r>>();
            }()
        ), ...);
    };
    instantiateRankMelds(std::make_index_sequence<N>{});
}

// Get the team's name
const std::string& Team::getName() const {
    return name;
}

// Add a player to the team (by reference)
void Team::addPlayer(Player& player) {
    // std::ref creates a reference_wrapper
    players.push_back(std::ref(player));
}

// Get the players in the team
const std::vector<std::reference_wrapper<Player>>& Team::getPlayers() const {
    return players;
}

// Get the team's melds (read-only access to the pointers)
const std::vector<std::unique_ptr<BaseMeld>>& Team::getMelds() const {
    return melds;
}

// Check if the team has made an initial meld
bool Team::hasMadeInitialMeld() const {
    // Check if any meld is initialized
    return std::any_of(melds.begin(), melds.end(),
        [](const std::unique_ptr<BaseMeld>& meldPtr) {
            return meldPtr && meldPtr->isInitialized();
        });
}


// Calculate the total points from the team's melds
int Team::calculateMeldPoints() const {
    // Use std::accumulate to sum the points from each meld
    return std::accumulate(melds.begin(), melds.end(), 0,
        [](int sum, const std::unique_ptr<BaseMeld>& meldPtr) {
            // Add the points from the current meld (if the pointer is valid)
            return sum + (meldPtr && meldPtr->isInitialized() ? meldPtr->getPoints() : 0);
        });
}

// Calculate the detailed score breakdown for the round
ScoreBreakdown Team::getScoreBreakdown() const {
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
            // Points from RedThreeMeld are purely bonus points
            breakdown.redThreeBonusPoints += meldTotalPoints;
            continue; // Skip further processing for this meld
        }

        int canastaBonus = 0;
        if (meldPtr->isCanastaMeld()) {
            std::optional<CanastaType> type = meldPtr->getCanastaType();
            if (type == CanastaType::Natural) {
                canastaBonus = NATURAL_CANASTA_BONUS;
                breakdown.naturalCanastaBonus += canastaBonus;
            } else if (type == CanastaType::Mixed) {
                canastaBonus = MIXED_CANASTA_BONUS;
                breakdown.mixedCanastaBonus += canastaBonus;
            }
            // else: Should not happen if isCanastaMeld is true, handle error?
        }

        // Card points are the total points minus any canasta bonus
        breakdown.meldedCardsPoints += (meldTotalPoints - canastaBonus);
    }

    // --- Calculate Hand Penalty ---
    int totalPenalty = 0;
    for (const auto& playerRef : players) {
        // Access the Player object through the reference_wrapper
        const Player& player = playerRef.get();
        totalPenalty += player.getHand().calculatePenalty();
    }
    // Penalty is stored as a negative value
    breakdown.handPenaltyPoints = -totalPenalty;

    return breakdown;
}

// Get a pointer to modify a specific rank-based meld (4-Ace)
BaseMeld* Team::getMeldForRank(Rank r) {
    auto indexOpt = getIndexForRank(r);
    if (indexOpt.has_value() && *indexOpt < melds.size() && melds[*indexOpt]) {
        return melds[*indexOpt].get(); // Return raw pointer
    }
    return nullptr; // Rank not valid or meld not initialized correctly
}


// Get a pointer to modify the Red Three meld
BaseMeld* Team::getRedThreeMeld() {
    if (RED_THREE_MELD_INDEX < melds.size() && melds[RED_THREE_MELD_INDEX]) {
        return melds[RED_THREE_MELD_INDEX].get();
    }
    return nullptr;
}

BaseMeld* Team::getBlackThreeMeld() {
    if (BLACK_THREE_MELD_INDEX < melds.size() && melds[BLACK_THREE_MELD_INDEX]) {
        return melds[BLACK_THREE_MELD_INDEX].get();
    }
    return nullptr;
}

// Note: The Cereal serialization function template is defined inline
// in the header file (team.hpp) and does not need a separate implementation here.
// Remember to handle player serialization (e.g., by name/ID) and
// register polymorphic Meld types with Cereal (CEREAL_REGISTER_TYPE) in a suitable .cpp file.