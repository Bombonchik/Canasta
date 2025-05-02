#include "hand.hpp"
#include <utility>

// Adds a card to the hand, maintaining sorted order.
// Requires Card::operator< to be defined.
void Hand::addCard(const Card& card) {
    // Find the correct insertion point to maintain sorted order
    auto it = std::lower_bound(cards.begin(), cards.end(), card);
    // Insert the card at the found position
    cards.insert(it, card);
}

void Hand::addCards(const std::vector<Card>& newCards, bool reversible) {
    // If reversible is true, store the current state of the hand for undo/redo
    if (reversible) {
        backupCards = cards; // Store a copy of the current state
        hasPendingReversible = true; // Set the flag indicating a reversible action
    }
    // Add cards to the hand
    for (const auto& card : newCards) {
        addCard(card);
    }
}

// Reverts the last reversible action by restoring the backup state.
void Hand::revertAddCards() {
    if (!hasPendingReversible) {
        throw std::logic_error("No reversible action to revert");
    }
    cards = std::move(backupCards); // Restore the previous state
    hasPendingReversible = false; // Reset the reversible action flag
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
const std::deque<Card>& Hand::getCards() const {
    return cards;
}

// Check if the hand contains a specific card.
bool Hand::hasCard(const Card& card) const {
    // Use std::find to check if the card exists in the hand
    return std::find(cards.begin(), cards.end(), card) != cards.end();
}

// Check if the hand is empty.
bool Hand::isEmpty() const {
    return cards.empty();
}

// Get the number of cards in the hand.
std::size_t Hand::cardCount() const {
    return cards.size();
}

// Calculate the total point value of cards remaining in the hand (penalty).
int Hand::calculatePenalty() const {
    int penalty = 0;
    for (const auto& card : cards) {
        penalty += card.getPoints();
    }
    // The result is typically treated as negative in final scoring,
    // but the function itself returns the positive sum.
    return penalty;
}

// Reset the hand.
void Hand::reset() {
    cards.clear();
    backupCards.clear();
    hasPendingReversible = false; // Reset the reversible action flag
}

// Note: The Cereal serialization function template is defined inline
// in the header file (hand.hpp) and does not need a separate implementation here.