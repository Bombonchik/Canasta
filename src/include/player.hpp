
#ifndef PLAYER_HPP
#define PLAYER_HPP
#include <string>
#include <cereal/types/string.hpp> // For string serialization
#include "hand.hpp" // Include the Hand class definition

/**
 * @class Player
 * @brief Class representing a player in the game.
 */
class Player {
public:
    /**
     * @brief Constructor to create a player with a given name.
     */
    explicit Player(std::string name);

    /**
     * @brief Default constructor for Player.
     * @details This constructor is used only for serialization and should not be used directly.
     */
    Player() = default;

    /**
     * @brief Get the name of the player.
     */
    const std::string& getName() const;

    /**
     * @brief Get a read-only reference to the player's hand.
     */
    const Hand& getHand() const;

    // Needed for dealing cards, player actions etc.
    /**
     * @brief Get a mutable reference to the player's hand.
     */
    Hand& getHand();

    // Reset the player's hand (e.g., at the start of a new round)
    /**
     * @brief Reset the player's hand to an empty state.
     * @details This method clears the hand and prepares it for a new round.
     */
    void resetHand();

    /**
     * @brief Serialize the Player object using Cereal.
     */
    template <class Archive>
    void serialize(Archive& ar) {
        ar(CEREAL_NVP(name), CEREAL_NVP(hand)); // Serialize name and hand
    }

private:
    std::string name;
    Hand hand;
};
#endif //PLAYER_HPP
