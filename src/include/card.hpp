

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

// Enum for Card Type
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

// Enum for Rank (Ace, Two, Three, ..., King)
enum class Rank {
    Joker = 1, Two, Three, Four, Five, Six, Seven,
    Eight, Nine, Ten, Jack, Queen, King, Ace
};

// Overload std::to_string for Rank
inline std::string to_string(Rank rank) {
    return rankNames[static_cast<int>(rank) - 1];
}

// Enum for Card Color (Red or Black)
enum class CardColor {
    RED, BLACK
};

// Overload std::to_string for CardColor
inline std::string to_string(CardColor color) {
    return colorNames[static_cast<int>(color)];
}

// Card Struct with Constructor, Getters, and Precomputed Points
class Card {
private:
    Rank rank;          // Card Rank (A, 2-10, J, Q, K, Joker)
    CardColor color;    // Suit Color (Red or Black)
    CardType type;      // Type (Natural, Wild, RedThree, BlackThree)
    int points;         // Precomputed points for efficiency

public:
    // Constructor
    Card(Rank rank, CardColor color);

    // **Getter Methods**
    Rank getRank() const { return rank; }
    CardType getType() const { return type; }
    CardColor getColor() const { return color; }
    int getPoints() const { return points; }
    std::string toString() const;

    bool operator==(const Card& other) const;
    bool operator!=(const Card& other) const;
    bool operator<(const Card& other) const;
    bool operator>(const Card& other) const;
    bool operator<=(const Card& other) const;
    bool operator>=(const Card& other) const;

    // Serialization with Cereal
    template <class Archive>
    void serialize(Archive& archive) {
        archive(rank, color, type, points);
    }

private:
    // Determine Card Type Automatically
    static CardType determineCardType(Rank rank, CardColor color);
    // Calculate Points
    static int calculatePoints(Rank rank, CardType type);
};



#endif //CARD_HPP
