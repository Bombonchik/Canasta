
#include "server/server_deck.hpp"
#include <stdexcept> // For std::runtime_error (optional, for errors)
#include <utility>   // For std::move
#include <cassert>   // For assert

// Constructor - Initializes and shuffles a 108-card Canasta deck
ServerDeck::ServerDeck(): isDiscardPileFrozen(false),
                        backupIsDiscardPileFrozen(false),
                        hasPendingReversible(false) {
    initializeMainDeck();
    shuffle();
    initializeDiscardPile();
}

// Helper to initialize a standard 108-card Canasta deck.
void ServerDeck::initializeMainDeck() {
    mainDeck.clear(); // Ensure deck is empty before initializing
    mainDeck.reserve(108); // Reserve space for efficiency

    // Initialize Two Full 52-Card Decks
    for (int deck = 0; deck < 2; ++deck) {
        for (int rankInt = static_cast<int>(Rank::Two); rankInt <= static_cast<int>(Rank::Ace); ++rankInt) {
            Rank rank = static_cast<Rank>(rankInt);
            // Add two suits of each rank (implicitly one red, one black)
            mainDeck.emplace_back(rank, CardColor::BLACK);
            mainDeck.emplace_back(rank, CardColor::RED);
            // Add the other two suits
            mainDeck.emplace_back(rank, CardColor::BLACK);
            mainDeck.emplace_back(rank, CardColor::RED);
        }
    }
    // Add 4 Jokers
    mainDeck.emplace_back(Rank::Joker, CardColor::RED);
    mainDeck.emplace_back(Rank::Joker, CardColor::RED);
    mainDeck.emplace_back(Rank::Joker, CardColor::BLACK);
    mainDeck.emplace_back(Rank::Joker, CardColor::BLACK);

    if (mainDeck.size() != 108) {
        // Handle error: throw exception or log
        throw std::runtime_error("Deck initialization failed: Incorrect card count.");
    }
}

// Initialize the discard pile
void ServerDeck::initializeDiscardPile() {
    discardPile.clear(); // Ensure discard pile is empty before initializing
    // Draw cards until a non-Red Three is found for the initial discard
    while (true) {
        std::optional<Card> topCardOpt = drawCard();
        assert(topCardOpt.has_value() && "Deck should not be empty when initializing discard pile");
        Card topCard = *topCardOpt;

        if (topCard.getType() == CardType::RedThree) {
            // If a Red Three is drawn, place it aside (conceptually) and draw again.
            continue; // Skip adding Red Three to the discard pile
        } else {
            discardCard(topCard);
            if (topCard.getType() == CardType::Natural) {
                break;
            }
        }
    }
}

// Shuffle the main deck
void ServerDeck::shuffle() {
    std::random_device rd;
    std::mt19937 g(rd());
    std::shuffle(mainDeck.begin(), mainDeck.end(), g);
}

// Draw a card from the main deck
std::optional<Card> ServerDeck::drawCard() {
    if (mainDeck.empty()) {
        return std::nullopt;
    }

    Card drawnCard = mainDeck.back();
    mainDeck.pop_back();
    return drawnCard;
}

// Add a card to the discard pile
void ServerDeck::discardCard(const Card& card) {
    discardPile.push_back(card);
    // Freeze the pile if a Black Three or a wild card (Joker, Two) is discarded
    CardType type = card.getType();
    if (type == CardType::BlackThree || type == CardType::Wild) {
        freezePile();
    }
}

// Get the top card of the discard pile without removing it
std::optional<Card> ServerDeck::getTopDiscard() const {
    //return (discardPile.empty()) ? std::nullopt : std::optional(discardPile.back());
    if (discardPile.empty()) {
        return std::nullopt;
    }
    return discardPile.back();
}

// Take the entire discard pile. Returns the pile and clears it. Unfreezes the pile internally.
// Returns an empty vector if the pile is empty.
std::expected<std::vector<Card>, std::string>
ServerDeck::takeDiscardPile(bool reversible) {
    if (discardPile.empty()) {
        return std::unexpected("Discard pile is empty");
    }

    // Check the top card
    const Card& topCard = discardPile.back();
    CardType topType = topCard.getType();

    if (topType == CardType::BlackThree || topType == CardType::Wild) {
        return std::unexpected("Cannot take discard pile: it is frozen");
    }

    // backup if requested
    if (reversible) {
        backupDiscardPile = discardPile;  // copy
        backupIsDiscardPileFrozen = isDiscardPileFrozen;
        hasPendingReversible = true;
    }
    // Move semantics efficiently transfer ownership of the vector's contents
    std::vector<Card> takenPile = std::move(discardPile);
    // discardPile is now guaranteed to be empty after the move
    discardPile.clear(); // Explicitly clear just in case (though move should suffice)
    unfreezePile();      // Taking the pile always unfreezes it
    return takenPile;
}

// Revert the last discard pile action (if reversible).
void ServerDeck::revertTakeDiscardPile() {
    if (!hasPendingReversible) {
        throw std::logic_error("No reversible action to revert");
    }
    discardPile = std::move(backupDiscardPile);
    isDiscardPileFrozen = backupIsDiscardPileFrozen;
    hasPendingReversible = false;
}

// Check if the main deck is empty.
bool ServerDeck::isMainDeckEmpty() const {
    return mainDeck.empty();
}

// Check if the discard pile is empty.
bool ServerDeck::isDiscardPileEmpty() const {
    return discardPile.empty();
}

// Get the number of cards remaining in the main deck.
size_t ServerDeck::mainDeckSize() const {
    return mainDeck.size();
}

// Get the number of cards in the discard pile.
size_t ServerDeck::discardPileSize() const {
    return discardPile.size();
}

// Check if the discard pile is physically frozen (due to wild card/red three).
bool ServerDeck::isFrozen() const {
    return isDiscardPileFrozen;
}

// --- Private Methods ---

// Explicitly freeze the discard pile.
void ServerDeck::freezePile() {
    isDiscardPileFrozen = true;
}

// Explicitly unfreeze the discard pile.
void ServerDeck::unfreezePile() {
    isDiscardPileFrozen = false;
}