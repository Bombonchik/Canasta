

#ifndef TEAM_HPP
#define TEAM_HPP

#include <vector>
#include <string>
#include <memory>       // For std::unique_ptr
#include <functional>   // For std::reference_wrapper
#include <numeric>      // For std::accumulate
#include <cereal/types/vector.hpp>
#include <cereal/types/string.hpp>
#include <cereal/types/memory.hpp> // For unique_ptr serialization
#include <cereal/types/polymorphic.hpp> // For polymorphic serialization
#include "player.hpp"
#include "meld.hpp"
#include "score_details.hpp"

/**
 * @class Team
 * @brief Class representing a team of players (1 or 2 players).
 */
class Team {
public:
    /**
     * @brief Constructor to create a team with a given name.
     */
    explicit Team(std::string name);

    /**
     * @brief Get the name of the team.
     */
    const std::string& getName() const;

    /**
     * @brief Add a player to the team.
     * @param player Reference to the player to add.
     */
    void addPlayer(Player& player);

    /**
     * @brief Check if the team has a specific player.
     * @param player Reference to the player to check.
     * @return True if the player is in the team, false otherwise.
     */
    bool hasPlayer(const Player& player) const;

    /**
     * @brief Get the players in the team.
     * @return A vector of references to the players in the team.
     */
    const std::vector<std::reference_wrapper<Player>>& getPlayers() const;


    /**
     * @brief Get the total score of the team.
     */
    int getTotalScore() const;

    /**
     * @brief Add points to the team's total score.
     * @param points The points to add.
     */
    void addToTotalScore(int points);

private:
    std::string name;

    int totalScore; // Total score for the game
    /**
     * @brief Vector of players in the team.
     * @details Players are stored as reference wrappers to avoid ownership issues.
     */
    std::vector<std::reference_wrapper<Player>> players;
};

#endif //TEAM_HPP