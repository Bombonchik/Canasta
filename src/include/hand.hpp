#ifndef HAND_HPP
#define HAND_HPP

#include <vector>
#include <cereal/types/vector.hpp> // Include for vector serialization
#include <algorithm> // Needed for std::lower_bound, std::find
#include "card.hpp" // Assuming card.hpp defines the Card class

class Hand {
public:
    // Constructor
    Hand() = default; // Use default constructor

    // Add a card to the hand
    void addCard(const Card& card);

    // Remove a card from the hand - returns true if successful, false otherwise
    // Note: This needs careful implementation based on how cards are identified (e.g., by value/suit or unique ID if added later)
    bool removeCard(const Card& card);

    // Get all cards in the hand
    const std::vector<Card>& getCards() const;

    // // Get a mutable reference to the cards (use with caution)
    // std::vector<Card>& getCards();

    // Check if the hand is empty
    bool isEmpty() const;

    // Get the number of cards in the hand
    size_t cardCount() const;

    // Clear the hand
    void clear();

    // Cereal serialization function
    template <class Archive>
    void serialize(Archive& ar) {
        ar(CEREAL_NVP(cards)); // Serialize the cards vector
    }

private:
    // Internal helper to find the insertion point for a card to maintain order.
    // std::vector<Card>::iterator findInsertionPoint(const Card& card); // Could be used by addCard

    std::vector<Card> cards; // Stores cards, kept sorted by Card::operator<
};

#endif // HAND_HPP