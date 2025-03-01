#include "card.hpp"

// **Constructor Implementation**
Card::Card(Rank rank, CardColor color)
    : rank(rank), color(color), type(determineCardType(rank, color)), points(calculatePoints(rank, type)) {}

// **Determine Card Type Automatically**
CardType Card::determineCardType(Rank rank, CardColor color) {
    if (rank == Rank::Joker || rank == Rank::Two) return CardType::Wild;
    if (rank == Rank::Three) return (color == CardColor::RED) ? CardType::RedThree : CardType::BlackThree;
    return CardType::Natural; // Default for all other cards
}

// **Calculate Points**
int Card::calculatePoints(Rank rank, CardType type) {
    switch (type) {
        case CardType::RedThree: return 100;  // Special bonus points
        case CardType::Wild: return (rank == Rank::Joker) ? 50 : 20; // Joker = 50, 2 = 20
        case CardType::BlackThree: return 5;  // Black threes (blocking)
        default:
            // Standard scoring for normal cards
                if (rank == Rank::Ace) return 20;
        if (rank >= Rank::Eight) return 10;  // 8-K
        return 5;  // 4-7
    }
}

bool Card::operator==(const Card& other) const {
    return rank == other.rank && color == other.color;
}

bool Card::operator!=(const Card& other) const {
    return !(*this == other);
}

bool Card::operator<(const Card& other) const {
    return rank < other.rank || (rank == other.rank && color < other.color);
}

bool Card::operator>(const Card& other) const {
    return rank > other.rank || (rank == other.rank && color > other.color);
}

bool Card::operator<=(const Card& other) const {
    return !(*this > other);
}

bool Card::operator>=(const Card& other) const {
    return !(*this < other);
}

