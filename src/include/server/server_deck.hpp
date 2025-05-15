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


/**
 *  @class ServerDeck
 *  @brief Class representing the main deck and discard pile in a Canasta game.
 */
class ServerDeck {
private:
    std::vector<Card> mainDeck;    // Full main deck (hidden from players)
    std::vector<Card> discardPile; // Discard pile (fully visible to players)
    std::vector<Card> backupDiscardPile; // Backup discard pile (for undo)
    bool isDiscardPileFrozen;         // Whether the discard pile is frozen
    bool backupIsDiscardPileFrozen; // Backup discard pile frozen state
    bool hasPendingReversible;      // Whether there is a pending reversible action
    /**
     * @brief Shuffles the main deck.
     */
    void shuffle();

    /**
     * @brief Initializes a standard 108-card Canasta main deck.
     */
    void initializeMainDeck();

    /**
     * @brief Initializes the discard pile.
     */
    void initializeDiscardPile();

    // --- Pile Management ---

    // Explicitly freeze the discard pile.
    void freezePile();

    // Explicitly unfreeze the discard pile.
    void unfreezePile();
public:
    /**
     * @brief Constructor for ServerDeck.
     * @details Initializes a standard 108-card Canasta main deck, shuffles it and initializes discard pile.
     */
    ServerDeck();

    /**
     * @brief Draw a card from the main deck.
     * @return An optional card. If the deck is empty, returns std::nullopt.
     */
    std::optional<Card> drawCard();

    /**
     * @brief Discard a card to the discard pile.
     * @param card The card to discard.
     * @details This method may freeze the discard pile if a Black Three or a wild card (Joker, Two) is discarded.
     */
    void discardCard(const Card& card);

    /**
     * @brief Take the entire discard pile if it isn't frozen.
     * @param reversible Indicates if the action is reversible.
     * @return A vector of cards from the discard pile. If the pile cannot be taken, returns an error message.
     */
    std::expected<std::vector<Card>, std::string>
    takeDiscardPile(bool reversible = false);

    // Revert the last discard pile action (if reversible).
    /**
     * @brief Revert the last reversible action on the discard pile.
     * @details This method restores the discard pile to its previous state.
     * @throws std::logic_error if there is no reversible action to revert.
     */
    void revertTakeDiscardPile();

    // --- State Queries ---

    /**
     * @brief Get the top card of the discard pile without removing it.
     * @return An optional card. If the discard pile is empty, returns std::nullopt.
     */
    std::optional<Card> getTopDiscard() const;

    // Check if the main deck is empty.
    bool isMainDeckEmpty() const;

    // Check if the discard pile is empty.
    bool isDiscardPileEmpty() const;

    // Get the number of cards remaining in the main deck.
    std::size_t mainDeckSize() const;

    // Get the number of cards in the discard pile.
    std::size_t discardPileSize() const;

    // Check if the discard pile is frozen.
    /**
     * @brief Check if the discard pile is frozen.
     */
    bool isFrozen() const;

    /**
     * @brief Serialize the ServerDeck object using Cereal.
     */
    template <class Archive>
    void serialize(Archive& archive) {
        archive(CEREAL_NVP(mainDeck), CEREAL_NVP(discardPile), CEREAL_NVP(isDiscardPileFrozen),
                CEREAL_NVP(backupDiscardPile), CEREAL_NVP(backupIsDiscardPileFrozen),
                CEREAL_NVP(hasPendingReversible));
    }
};

#endif //DECK_HPP

