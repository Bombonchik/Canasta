
#ifndef PLAYER_HPP
#define PLAYER_HPP
#include <string>
#include <cereal/types/string.hpp> // For string serialization
#include "hand.hpp" // Include the Hand class definition

class Player {
public:
    // Constructor - Takes a player name
    explicit Player(std::string name);

    // Default constructor (needed for some serialization/container use cases)
    Player() = default;

    // Get the player's name
    const std::string& getName() const;

    // Get read-only access to the player's hand
    const Hand& getHand() const;

    // Get mutable access to the player's hand
    // Needed for dealing cards, player actions etc.
    Hand& getHand();

    // Cereal serialization function
    template <class Archive>
    void serialize(Archive& ar) {
        ar(CEREAL_NVP(name), CEREAL_NVP(hand)); // Serialize name and hand
    }

private:
    std::string name;
    Hand hand;
};
#endif //PLAYER_HPP
