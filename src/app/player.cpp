#include "player.hpp"
#include <utility> // For std::move

// Constructor - Takes a player name and moves it into the member variable
Player::Player(std::string name) : name(std::move(name)) {
    // The hand member is default-initialized automatically
}

// Get the player's name
const std::string& Player::getName() const {
    return name;
}

// Get read-only access to the player's hand
const Hand& Player::getHand() const {
    return hand;
}

// Get mutable access to the player's hand
Hand& Player::getHand() {
    return hand;
}

// Reset the player's hand (e.g., at the start of a new round)
void Player::resetHand() {
    hand.reset(); // Reset the hand to its initial state
}

// Note: The Cereal serialization function template is defined inline
// in the header file (player.hpp) and does not need a separate implementation here.