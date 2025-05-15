

#ifndef CARD_HPP
#define CARD_HPP

#include <string>
#include <array>
#include <cereal/types/string.hpp>  // For serialization


constexpr unsigned CARD_COUNT = 14;
constexpr std::array<const char*, CARD_COUNT> rankNames = {
    "Joker", "Two", "Three", "Four", "Five", "Six", "Seven",
    "Eight", "Nine", "Ten", "Jack", "Queen", "King", "Ace"
};

constexpr unsigned CARD_COLOR_COUNT = 2;
constexpr std::array<const char*, CARD_COLOR_COUNT> colorNames = {
    "Red", "Black"
};

constexpr unsigned CARD_TYPE_COUNT = 4;
constexpr std::array<const char*, CARD_TYPE_COUNT> typeNames = {
    "Natural", "Wild", "Red Three", "Black Three"
};

/**
 * @enum CardType
 * @brief Enum representing the type of card.
 * @details Natural: Normal cards (4-7, 8-K, A)
 * @details Wild: Jokers and 2s
 * @details RedThree: Special scoring card
 * @details BlackThree: Blocks discard pile
 */
enum class CardType {
    Natural,   // Normal cards (4-7, 8-K, A)
    Wild,      // Jokers and 2s
    RedThree,  // Special scoring card
    BlackThree // Blocks discard pile
};

// Overload std::to_string for CardType
inline std::string to_string(CardType type) {
    return typeNames[static_cast<int>(type)];
}

/**
 * @enum Rank
 * @brief Enum representing the rank of a card.
 */
enum class Rank {
    Joker = 1, Two, Three, Four, Five, Six, Seven,
    Eight, Nine, Ten, Jack, Queen, King, Ace
};

// Overload std::to_string for Rank
inline std::string to_string(Rank rank) {
    return rankNames[static_cast<int>(rank) - 1];
}

/**
 * @enum CardColor
 * @brief Enum representing the color of a card.
 */
enum class CardColor {
    RED, BLACK
};

// Overload std::to_string for CardColor
inline std::string to_string(CardColor color) {
    return colorNames[static_cast<int>(color)];
}

/**
 * @class Card
 * @brief Represents a playing card with rank, color, and type.
 */
class Card {
private:
    Rank rank;          // Card Rank (A, 2-10, J, Q, K, Joker)
    CardColor color;    // Suit Color (Red or Black)
    CardType type;      // Type (Natural, Wild, RedThree, BlackThree)
    int points;         // Precomputed points for efficiency
public:

    /**
    * @brief Constructor for Card.
    * @param rank The rank of the card (1-14 for Joker, 2-10, J, Q, K, A).
    * @param color The color of the card (Red or Black).
    */
    Card(Rank rank, CardColor color);

    /**
     * @brief Constructor only for serialization purposes.
     */
    Card() = default;

    // **Getter Methods**

    /**
     * @brief Get the rank of the card.
     */
    Rank getRank() const { return rank; }
    /**
     * @brief Get the type of the card.
     */
    CardType getType() const { return type; }
    /**
     * @brief Get the color of the card.
     */
    CardColor getColor() const { return color; }
    /**
     * @brief Get the points of the card.
     */
    int getPoints() const { return points; }
    /**
     * @brief Get the string representation of the card.
     * @return A string representing the card (e.g., "Red Six").
     */
    std::string toString() const;

    bool operator==(const Card& other) const;
    bool operator!=(const Card& other) const;
    bool operator<(const Card& other) const;
    bool operator>(const Card& other) const;
    bool operator<=(const Card& other) const;
    bool operator>=(const Card& other) const;

    /**
     * @brief Serialize the Card object using Cereal.
     */
    template <class Archive>
    void serialize(Archive& archive) {
        archive(CEREAL_NVP(rank), CEREAL_NVP(color), CEREAL_NVP(type), CEREAL_NVP(points));
    }

private:
    /**
     * @brief Determine the type of the card based on its rank and color.
     */
    static CardType determineCardType(Rank rank, CardColor color);
    /**
     * @brief Calculate the points for the card based on its rank and type.
     * @param rank The rank of the card.
     * @param type The type of the card.
     * @return The points for the card.
     */
    static int calculatePoints(Rank rank, CardType type);
};

#endif //CARD_HPP
