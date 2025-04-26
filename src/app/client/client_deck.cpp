
#include "client/client_deck.hpp"
#include <utility> 

// Constructor to initialize from server-side data
ClientDeck::ClientDeck(std::optional<Card> topCard, size_t discardSize, size_t deckSize, bool isFrozen)
    :   topDiscardCard(topCard), // Directly initialize members
        discardPileSize(discardSize),
        mainDeckSize(deckSize),
        isDiscardPileFrozen(isFrozen)
{}