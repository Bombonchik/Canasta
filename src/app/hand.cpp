#include "hand.hpp"

// Adds a card to the hand, maintaining sorted order.
// Requires Card::operator< to be defined.
void Hand::addCard(const Card& card) {
    // Find the correct insertion point to maintain sorted order
    auto it = std::lower_bound(cards.begin(), cards.end(), card);
    // Insert the card at the found position
    cards.insert(it, card);
}

// Removes the first occurrence of a specific card instance from the hand.
// Returns true if a card was found and removed, false otherwise.
// Requires Card::operator== to be defined.
bool Hand::removeCard(const Card& card) {
    // Find the first card that matches the given card
    auto it = std::find(cards.begin(), cards.end(), card);

    // If the card was found
    if (it != cards.end()) {
        // Erase the card from the vector
        cards.erase(it);
        return true; // Indicate success
    }
    return false; // Indicate card not found
}

// Get read-only access to all cards in the hand (cards are kept sorted).
const std::vector<Card>& Hand::getCards() const {
    return cards;
}

// Check if the hand is empty.
bool Hand::isEmpty() const {
    return cards.empty();
}

// Get the number of cards in the hand.
size_t Hand::cardCount() const {
    return cards.size();
}

// Clear the hand.
void Hand::clear() {
    cards.clear();
}

// Note: The Cereal serialization function template is defined inline
// in the header file (hand.hpp) and does not need a separate implementation here.