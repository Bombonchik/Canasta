

#ifndef CARD_HPP
#define CARD_HPP

#include <cereal/types/string.hpp>  // For serialization

// Enum for Card Type (Natural, Wild, Special)
enum class CardType {
    Natural,   // Normal cards (4-7, 8-K, A)
    Wild,      // Jokers and 2s
    RedThree,  // Special scoring card
    BlackThree // Blocks discard pile
};

// Enum for Rank (Ace, Two, Three, ..., King)
enum class Rank {
    Joker = 1, Two, Three, Four, Five, Six, Seven,
    Eight, Nine, Ten, Jack, Queen, King, Ace
};

// Enum for Card Color (Red or Black)
enum class CardColor {
    RED, BLACK
};

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
