#ifndef TEAM_ROUND_STATE_HPP
#define TEAM_ROUND_STATE_HPP

#include <vector>
#include <string>
#include <memory>       // For std::unique_ptr
#include <functional>   // For std::reference_wrapper
#include <numeric>      // For std::accumulate
#include <cereal/types/vector.hpp>
#include <cereal/types/string.hpp>
#include <cereal/types/memory.hpp> // For unique_ptr serialization
#include <cereal/types/polymorphic.hpp> // For polymorphic serialization
#include "meld.hpp"
#include "score_details.hpp"


class TeamRoundState {
public:
    TeamRoundState();
    // --- Meld Management ---

    // Get read-only access to the collection of all potential meld objects
    const std::vector<std::unique_ptr<BaseMeld>>& getMelds() const;

    // Check if the team has made an initial meld (any meld is initialized)
    bool hasMadeInitialRankMeld() const;

    // Calculate the total points from the team's melds
    int calculateMeldPoints() const;

    // Calculate the detailed score breakdown for the round
    ScoreBreakdown getScoreBreakdown(int goingOutBonus) const;

    // Get a pointer to modify a specific rank-based meld (4-Ace)
    // Returns nullptr if the rank is invalid for a standard meld.
    BaseMeld* getMeldForRank(Rank r);

    // Get a pointer to modify the Red Three meld
    BaseMeld* getRedThreeMeld();

    // Get a pointer to modify the Black Three meld
    BaseMeld* getBlackThreeMeld();

    // Const overload (for read-only callers, like RuleEngine)
    const BaseMeld* getMeldForRank(Rank r) const;

    // Const overload for Black Three Meld
    const BaseMeld* getBlackThreeMeld() const;

    // Const overload for Red Three Meld
    const BaseMeld* getRedThreeMeld() const;

    void reset();

    TeamRoundState clone() const;

    // Cereal serialization function
    template <class Archive>
    void serialize(Archive& ar) {
        // Placeholder: Serialization of players needs decision (e.g., serialize names/IDs)
        ar(CEREAL_NVP(melds));
        // Note: Polymorphic serialization requires registration of derived types (e.g., using CEREAL_REGISTER_TYPE)
    }
private:
    // Helper to pre-create all 13 possible meld types
    void createMelds();

    // Helper to get index for a given rank (internal use)
    std::optional<size_t> getIndexForRank(Rank r) const;

    // Storing unique pointers to BaseMeld to handle polymorphism
    std::vector<std::unique_ptr<BaseMeld>> melds;
};

#endif // TEAM_ROUND_STATE_HPP