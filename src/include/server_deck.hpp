#ifndef SERVER_DECK_HPP
#define SERVER_DECK_HPP

#include "card.hpp"
#include <vector>
#include <algorithm>
#include <random>
#include <cereal/types/vector.hpp>
#include <cereal/access.hpp>
#include <optional>
#include <expected>


// Deck class
class ServerDeck {
private:
    std::vector<Card> mainDeck;    // Full main deck (hidden from players)
    std::vector<Card> discardPile; // Discard pile (fully visible to players)
    std::vector<Card> backupDiscardPile; // Backup discard pile (for undo)
    bool isDiscardPileFrozen;         // Whether the discard pile is frozen
    bool backupIsDiscardPileFrozen; // Backup discard pile frozen state
    bool hasPendingReversible;      // Whether there is a pending reversible action
    // Shuffle the main deck
    void shuffle();

    // Helper to initialize a standard 108-card Canasta deck.
    void initializeDeck();

    // --- Pile Management ---

    // Explicitly freeze the discard pile.
    void freezePile();

    // Explicitly unfreeze the discard pile.
    void unfreezePile();
public:
    // Constructor - Initializes and shuffles a 108-card Canasta deck
    ServerDeck();

    // Draw a card from the main deck
    std::optional<Card> drawCard();

    // Add a card to the discard pile. May freeze the pile.
    void discardCard(const Card& card);

    // Take the entire discard pile. Returns the pile and clears it. Unfreezes the pile.
    // Returns an empty vector if the pile cannot be taken (e.g., empty).
    std::expected<std::vector<Card>, std::string>
    takeDiscardPile(bool reversible = false);

    // Revert the last discard pile action (if reversible).
    void revertTakeDiscardPile();

    // --- State Queries ---

    // Get the top card of the discard pile
    std::optional<Card> getTopDiscard() const;

    // Check if the main deck is empty.
    bool isMainDeckEmpty() const;

    // Check if the discard pile is empty.
    bool isDiscardPileEmpty() const;

    // Get the number of cards remaining in the main deck.
    size_t mainDeckSize() const;

    // Get the number of cards in the discard pile.
    size_t discardPileSize() const;

    // Check if the discard pile is frozen.
    bool isFrozen() const;

    // Serialization
    template <class Archive>
    void serialize(Archive& archive) {
        archive(CEREAL_NVP(mainDeck), CEREAL_NVP(discardPile), CEREAL_NVP(isDiscardPileFrozen),
                CEREAL_NVP(backupDiscardPile), CEREAL_NVP(backupIsDiscardPileFrozen),
                CEREAL_NVP(hasPendingReversible));
    }
};

#endif //DECK_HPP

