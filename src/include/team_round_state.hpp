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


/**
 * @class TeamRoundState
 * @brief Represents the state of a team's melds and scores in a round.
 */
class TeamRoundState {
public:
    /**
     * @brief Default constructor for TeamRoundState.
     * @details Precreates all possible melds for the team.
     */
    TeamRoundState();
    // --- Meld Management ---

    /**
     * @brief Get the team's melds.
     * @return A vector of unique pointers to BaseMeld objects.
     */
    const std::vector<std::unique_ptr<BaseMeld>>& getMelds() const;

    /**
     * @brief Check if the team has made an initial rank meld.
     * @return True if any rank meld is initialized, false otherwise.
     */
    bool hasMadeInitialRankMeld() const;

    // Calculate the total points from the team's melds
    /**
     * @brief Calculate the total points from the team's melds.
     * @return The total points from all melds (excluding Red Three).
     */
    int calculateMeldPoints() const;

    /**
     * @brief Calculate the score breakdown for the round.
     */
    ScoreBreakdown getScoreBreakdown(int goingOutBonus) const;

    /**
     * @brief Get a pointer to modify a specific rank-based meld.
     * @param r The rank of the meld to get.
     * @return A pointer to the BaseMeld object for the specified rank or nullptr if invalid.
     */
    BaseMeld* getMeldForRank(Rank r);

    /**
     * @brief Get a pointer to modify the Red Three meld.
     * @return A pointer to the BaseMeld object for the Red Three meld or nullptr if invalid.
     */
    BaseMeld* getRedThreeMeld();

    /**
     * @brief Get a pointer to modify the Black Three meld.
     * @return A pointer to the BaseMeld object for the Black Three meld or nullptr if invalid.
     */
    BaseMeld* getBlackThreeMeld();

    /**
     * @brief Get a pointer to a specific rank-based meld (const version).
     * @param r The rank of the meld to get.
     * @return A const pointer to the BaseMeld object for the specified rank or nullptr if invalid.
     */
    const BaseMeld* getMeldForRank(Rank r) const;

    /**
     * @brief Get a pointer to the Red Three meld (const version).
     * @return A const pointer to the BaseMeld object for the Red Three meld or nullptr if invalid.
     */
    const BaseMeld* getBlackThreeMeld() const;

    /**
     * @brief Get a pointer to the Black Three meld (const version).
     * @return A const pointer to the BaseMeld object for the Black Three meld or nullptr if invalid.
     */
    const BaseMeld* getRedThreeMeld() const;

    /**
     * @brief Reset the state of the team round.
     * @details This function clears all melds and resets the state.
     */
    void reset();

    /**
     * @brief Clone the TeamRoundState object.
     * @return A new TeamRoundState object that is a copy of this one.
     */
    TeamRoundState clone() const;

    /**
     * @brief Serialize the TeamRoundState using Cereal.
     */
    template <class Archive>
    void serialize(Archive& ar) {
        // Placeholder: Serialization of players needs decision (e.g., serialize names/IDs)
        ar(CEREAL_NVP(melds));
        // Note: Polymorphic serialization requires registration of derived types (e.g., using CEREAL_REGISTER_TYPE)
    }
private:
    // Helper to pre-create all 13 possible meld types
    /**
     * @brief Creates all possible melds for the team.
     */
    void createMelds();

    /**
     * @brief Gets the index for a given rank.
     * @param r The rank to get the index for.
     * @return The index corresponding to the rank, or std::nullopt if invalid.
     */
    std::optional<std::size_t> getIndexForRank(Rank r) const;

    /**
     * @brief Vector of unique pointers to BaseMeld objects for all the melds.
     */
    std::vector<std::unique_ptr<BaseMeld>> melds;
};

#endif // TEAM_ROUND_STATE_HPP