
#include "meld.hpp"

// Implementation of RedThreeMeld::initialize
Status RedThreeMeld::initialize(const std::vector<Card>& cards) {
    if (auto status = checkInitialization(cards); !status.has_value()) {
        return status; // Forward the error message if check fails
    }
    redThreeCount = cards.size();
    isActive = true;
    updatePoints(); // Update points after initialization
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
    if (cards.size() > 4) {
        return std::unexpected("Red Three Meld can contain at most 4 cards");
    }
    for (const auto& card : cards) {
        if (card.getType() != CardType::RedThree) {
            return std::unexpected("Invalid card " + card.toString() + " for Red Three Meld");
        }
    }
    return {};
}

// Implementation of RedThreeMeld::addCard
Status RedThreeMeld::addCard(const Card& card) {
    if (auto status = checkCardAddition(card); !status.has_value()) {
        return status; // Forward the error message if check fails
    }
    redThreeCount++;
    updatePoints(); // Update points after adding a card
    return {};
}

// Implementation of RedThreeMeld::checkCardAddition
Status RedThreeMeld::checkCardAddition(const Card& card) const {
    if (!isActive) {
        return std::unexpected("Red Three Meld is not initialized");
    }
    if (card.getType() != CardType::RedThree) {
        return std::unexpected("Invalid card for Red Three Meld");
    }
    if (redThreeCount >= 4) {
        return std::unexpected("Red Three Meld can contain at most 4 cards");
    }
    return {};
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
Status BlackThreeMeld::initialize(const std::vector<Card>& cards) {
    if (auto status = checkInitialization(cards); !status.has_value()) {
        return status; // Forward the error message if check fails
    }
    blackThreeCount = cards.size();
    isActive = true;
    updatePoints(); // Update points after initialization
    return {};
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
Status BlackThreeMeld::addCard(const Card& card) {
    return std::unexpected("It is not possible to add card to Black Three Meld");
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

