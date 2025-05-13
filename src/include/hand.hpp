#ifndef HAND_HPP
#define HAND_HPP

#include <vector>
#include <deque> // Include for deque serialization
#include <cereal/types/vector.hpp> // Include for vector serialization
#include <cereal/types/deque.hpp> // Include for vector serialization
#include <algorithm> // Needed for std::lower_bound, std::find
#include "card.hpp" // Assuming card.hpp defines the Card class

/**
 * @class Hand
 * @brief Class representing a player's hand of cards.
 */
class Hand {
public:
    // Constructor
    Hand() = default; // Use default constructor

    /**
     * @brief Adds a card to the hand, maintaining sorted order.
     * @param card The card to add.
     */
    void addCard(const Card& card);

    /**
     * @brief Adds multiple cards to the hand, maintaining sorted order.
     * @param newCards The cards to add.
     * @param reversible Indicates if the addition is reversible.
     */
    void addCards(const std::vector<Card>& newCards, bool reversible = false);

    /**
     * @brief Reverts the last reversible action by restoring the backup state.
     * @throws std::logic_error if there is no reversible action to revert.
     */
    void revertAddCards();

    // Remove a card from the hand - returns true if successful, false otherwise
    /**
     * @brief Removes a card from the hand.
     * @param card The card to remove.
     */
    bool removeCard(const Card& card);

    // Get all cards in the hand
    /**
     * @brief Get a read-only reference to all cards in the hand.
     * @return A const reference to the deque of cards.
     */
    const std::deque<Card>& getCards() const;

    /**
     * @brief Checks if the hand contains a specific card.
     * @param card The card to check for.
     * @return True if the card is in the hand, false otherwise.
     */
    bool hasCard(const Card& card) const;

    /**
     * * @brief Checks if the hand is empty.
     */
    bool isEmpty() const;

    /**
     * * @brief Gets the number of cards in the hand.
     */
    std::size_t cardCount() const;

    /**
     * * @brief Resets the hand to its initial state.
     */
    void reset();

    /**
     * * @brief Calculates the total point value of cards remaining in the hand (penalty).
     */
    int calculatePenalty() const;

    /**
     * * @brief Serialize the Hand using Cereal.
     */
    template <class Archive>
    void serialize(Archive& ar) {
        ar(CEREAL_NVP(cards), CEREAL_NVP(backupCards), CEREAL_NVP(hasPendingReversible)); // Serialize the cards vector
    }

private:

    /**
     * @brief contains a card in the hand.
     * @details it's important that it is deque<Card> and not vector<Card>
     */
    std::deque<Card> cards; // Stores cards, kept sorted by Card::operator<
    std::deque<Card> backupCards; // Backup for undo/redo operations
    bool hasPendingReversible = false; // Whether there is a pending reversible action
};

#endif // HAND_HPP