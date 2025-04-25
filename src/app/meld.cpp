
#include "meld.hpp"

// Implementation of RedThreeMeld::initialize
void RedThreeMeld::initialize(const std::vector<Card>& cards) {
    auto status = checkInitialization(cards);
    assert(status.has_value() && "Red Three Meld initialization failed");
    redThreeCount = cards.size();
    isActive = true;
    updatePoints(); // Update points after initialization
}

// Implementation of RedThreeMeld::validateCards
Status RedThreeMeld::validateCards(const std::vector<Card>& cards, std::size_t redThreeCount) const {
    if (cards.size() + redThreeCount > 4) {
        return std::unexpected("Red Three Meld can contain at most 4 cards");
    }
    for (const auto& card : cards) {
        if (card.getType() != CardType::RedThree) {
            return std::unexpected("Invalid card " + card.toString() + " for Red Three Meld");
        }
    }
    return {};
}

// Implementation of RedThreeMeld::checkInitialization
Status RedThreeMeld::checkInitialization(const std::vector<Card>& cards) const {
    if (isActive) {
        return std::unexpected("Red Three Meld is already initialized");
    }
    if (cards.empty()) {
        return std::unexpected("Red Three Meld must contain at least 1 card");
    }
    return validateCards(cards);
}

// Implementation of RedThreeMeld::addCard
void RedThreeMeld::addCards(const std::vector<Card>& cards) {
    auto status = checkCardsAddition(cards);
    assert(status.has_value() && "Red Three Meld addition failed");
    redThreeCount++;
    updatePoints(); // Update points after adding a card
}

// Implementation of RedThreeMeld::checkCardAddition
Status RedThreeMeld::checkCardsAddition(const std::vector<Card>& cards) const {
    if (!isActive) {
        return std::unexpected("Red Three Meld is not initialized");
    }
    if (cards.empty()) {
        return std::unexpected("You must add at least 1 card");
    }
    return validateCards(cards, redThreeCount);
}

// Implementation of RedThreeMeld::getPoints
int RedThreeMeld::getPoints() const {
    // Simply return the cached value
    return points;
}

// Implementation of RedThreeMeld::updatePoints
void RedThreeMeld::updatePoints() {
    const int redThreePointsValue = Card(Rank::Three, CardColor::RED).getPoints();
    int calculatedPoints = redThreeCount * redThreePointsValue;
    if (redThreeCount == 4) {
        calculatedPoints *= 2; // double points for a complete Red Three Meld
    }
    points = calculatedPoints; // Update the member variable
}


// Implementation of BlackThreeMeld::initialize
void BlackThreeMeld::initialize(const std::vector<Card>& cards) {
    auto status = checkInitialization(cards);
    assert(status.has_value() && "Black Three Meld initialization failed");
    blackThreeCount = cards.size();
    isActive = true;
    updatePoints(); // Update points after initialization
}

// Implementation of BlackThreeMeld::checkInitialization
Status BlackThreeMeld::checkInitialization(const std::vector<Card>& cards) const {
    if (isActive) {
        return std::unexpected("Black Three Meld is already initialized");
    }
    if (cards.size() < 3) {
        return std::unexpected("Black Three Meld must contain at least 3 cards");
    }
    if (cards.size() > 4) {
        return std::unexpected("Black Three Meld can contain at most 4 cards");
    }
    for (const auto& card : cards) {
        if (card.getType() != CardType::BlackThree) {
            return std::unexpected("Invalid card " + card.toString() + " for Black Three Meld");
        }
    }
    return {};
}

// Implementation of BlackThreeMeld::addCard
void BlackThreeMeld::addCards(const std::vector<Card>& cards) {
    throw std::logic_error("BlackThreeMeld: addCards() unsupported");
}

// Implementation of BlackThreeMeld::checkCardAddition
Status BlackThreeMeld::checkCardsAddition(const std::vector<Card>& cards) const {
    return std::unexpected("Black Three Meld does not support adding cards");
}

// Implementation of BlackThreeMeld::getPoints
int BlackThreeMeld::getPoints() const {
    // Simply return the cached value
    return points;
}

// Implementation of BlackThreeMeld::updatePoints
void BlackThreeMeld::updatePoints() {
    points = blackThreeCount * Card(Rank::Three, CardColor::BLACK).getPoints();
}

