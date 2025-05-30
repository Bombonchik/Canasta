#include "team.hpp"
#include <utility> // For std::move
#include <stdexcept> // For potential errors

// Constructor - Takes a team name and moves it into the member variable
Team::Team(const std::string& name) : name(name), totalScore(0) {}

// Get the team's name
const std::string& Team::getName() const {
    return name;
}

// Add a player to the team (by reference)
void Team::addPlayer(Player& player) {
    // std::ref creates a reference_wrapper
    players.push_back(std::ref(player));
}

// Check if the team has a specific player
bool Team::hasPlayer(const Player& player) const {
    // Check if the player is in the team
    return std::any_of(players.begin(), players.end(),
        [&](const std::reference_wrapper<Player>& p) { return p.get().getName() == player.getName(); });
}

// Get the players in the team
const std::vector<std::reference_wrapper<Player>>& Team::getPlayers() const {
    return players;
}

// Get the total score of the team
int Team::getTotalScore() const {
    return totalScore;
}

// Add points to the team's total score
void Team::addToTotalScore(int points) {
    totalScore += points;
}

// Note: The Cereal serialization function template is defined inline
// in the header file (team.hpp) and does not need a separate implementation here.
// Remember to handle player serialization (e.g., by name/ID) and