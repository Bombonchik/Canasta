#ifndef CLIENT_DECK_HPP
#define CLIENT_DECK_HPP

#include <optional>
#include <cereal/types/optional.hpp> // Include for optional serialization
#include <cereal/access.hpp>
#include "card.hpp" // Include Card definition

// Represents the view of the deck state visible to a client (DTO).
/**
 * @struct ClientDeck
 * @brief Represents the ServerDeck state visible to a client.
 * @details This struct is used to send deck information to the client.
 */
struct ClientDeck {
    /**
     * @brief Number of cards left in the main deck.
     */
    std::size_t mainDeckSize = 0;
    /**
     * @brief Top card of the discard pile (if any).
     * @details This is an optional field, as there may be no cards in the discard pile.
     */
    std::optional<Card> topDiscardCard;
    /**
     * @brief Number of cards in the discard pile.
     */
    std::size_t discardPileSize = 0;
    /**
     * @brief Indicates if the discard pile is frozen.
     * @details The discard pile can be frozen by certain cards (e.g., Black Three or Wild).
     */
    bool isDiscardPileFrozen = false;

    /**
     * @brief Default constructor for ClientDeck only for serialization purposes.
     */
    ClientDeck() = default;

    /**
     * @brief Constructor to initialize ClientDeck from server-side data.
     * @param deckSize Size of the main deck.
     * @param topCard Top card of the discard pile (if any).
     * @param discardSize Size of the discard pile.
     * @param isFrozen Indicates if the discard pile is frozen.
     */
    explicit ClientDeck(std::size_t deckSize, std::optional<Card> topCard, std::size_t discardSize, bool isFrozen);

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