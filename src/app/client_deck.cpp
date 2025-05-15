
#include "client_deck.hpp"
#include <utility> 

// Constructor to initialize from server-side data
ClientDeck::ClientDeck(std::size_t deckSize, std::optional<Card> topCard, std::size_t discardSize, bool isFrozen)
    :   mainDeckSize(deckSize),
        topDiscardCard(topCard),
        discardPileSize(discardSize),
        isDiscardPileFrozen(isFrozen)
{}