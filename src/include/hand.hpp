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

    void addCards(const std::vector<Card>& newCards, bool reversible = false);

    void revertAddCards();

    // Remove a card from the hand - returns true if successful, false otherwise
    // Note: This needs careful implementation based on how cards are identified (e.g., by value/suit or unique ID if added later)
    bool removeCard(const Card& card);

    // Get all cards in the hand
    const std::vector<Card>& getCards() const;

    // // Get a mutable reference to the cards (use with caution)
    // std::vector<Card>& getCards();

    bool hasCard(const Card& card) const;

    // Check if the hand is empty
    bool isEmpty() const;

    // Get the number of cards in the hand
    std::size_t cardCount() const;

    // Reset the hand
    void reset();

    // Calculate the total point value of cards remaining in the hand (penalty)
    int calculatePenalty() const;

    // Cereal serialization function
    template <class Archive>
    void serialize(Archive& ar) {
        ar(CEREAL_NVP(cards), CEREAL_NVP(backupCards), CEREAL_NVP(hasPendingReversible)); // Serialize the cards vector
    }

private:
    // Internal helper to find the insertion point for a card to maintain order.
    // std::vector<Card>::iterator findInsertionPoint(const Card& card); // Could be used by addCard

    std::vector<Card> cards; // Stores cards, kept sorted by Card::operator<
    std::vector<Card> backupCards; // Backup for undo/redo operations
    bool hasPendingReversible = false; // Whether there is a pending reversible action
};

#endif // HAND_HPP