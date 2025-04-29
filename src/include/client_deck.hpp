#ifndef CLIENT_DECK_HPP
#define CLIENT_DECK_HPP

#include <optional>
#include <cereal/types/optional.hpp> // Include for optional serialization
#include <cereal/access.hpp>
#include "card.hpp" // Include Card definition

// Represents the view of the deck state visible to a client (DTO).
struct ClientDeck {
    // Public members for easy access as a DTO
    std::size_t mainDeckSize = 0;            // Number of cards left in main deck
    std::optional<Card> topDiscardCard;     // Top card of the discard pile (if any)
    std::size_t discardPileSize = 0;         // Number of cards in the discard pile
    bool isDiscardPileFrozen = false;   // Is the pile frozen by Black Three/Wild?

    // Default constructor (needed for serialization/deserialization)
    ClientDeck() = default;

    // Constructor to initialize from server-side data (optional, but can be useful)
    ClientDeck(std::size_t deckSize, std::optional<Card> topCard, std::size_t discardSize, bool isFrozen);

    // --- Getters (Optional if members are public) ---
    const std::optional<Card>& getTopDiscardCard() const { return topDiscardCard; }
    std::size_t getDiscardPileSize() const { return discardPileSize; }
    std::size_t getMainDeckSize() const { return mainDeckSize; }
    bool isFrozen() const { return isDiscardPileFrozen; }

    // --- Serialization ---
    template <class Archive>
    void serialize(Archive& archive) {
        archive(CEREAL_NVP(topDiscardCard), CEREAL_NVP(discardPileSize), CEREAL_NVP(mainDeckSize), CEREAL_NVP(isDiscardPileFrozen));
    }
};

#endif // CLIENT_DECK_HPP