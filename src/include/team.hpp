

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


class Team {
public:
    // Constructor - Takes a team name
    explicit Team(std::string name);

    // Get the team's name
    const std::string& getName() const;

    // Add a player to the team (by reference)
    void addPlayer(Player& player);

    // Get the players in the team
    const std::vector<std::reference_wrapper<Player>>& getPlayers() const;

    int getTotalScore() const;

    void addToTotalScore(int points);

    // Cereal serialization function
    template <class Archive>
    void serialize(Archive& ar) {
        // Placeholder: Serialization of players needs decision (e.g., serialize names/IDs)
        ar(CEREAL_NVP(name));
    }

private:
    std::string name;

    int totalScore; // Total score for the game
    // Using reference_wrapper to refer to Player objects owned elsewhere
    std::vector<std::reference_wrapper<Player>> players;
    // Storing unique pointers to BaseMeld to handle polymorphism
};

#endif //TEAM_HPP