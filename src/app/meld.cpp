
#include "meld.hpp"


// Implementation of RedThreeMeld::initialize
Status RedThreeMeld::initialize(const std::vector<Card>& cards) {
    if (auto status = checkInitialization(cards); !status.has_value()) {
        return status; // Forward the error message if check fails
    }
    redThreeCount = cards.size();
    isActive = true;

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

// Implementation of RedThreeMeld::getTotalPoints
int RedThreeMeld::updateTotalPoints() {
    points = 0;
    const int redThreePoints = Card(Rank::Three, CardColor::RED).getPoints();
    points = redThreeCount * redThreePoints;
    if (redThreeCount == 4) {
        points *= 2; // double points for a complete Red Three Meld
    }

    return points;
}

// Implementation of BlackThreeMeld::initialize
Status BlackThreeMeld::initialize(const std::vector<Card>& cards) {
    if (auto status = checkInitialization(cards); !status.has_value()) {
        return status; // Forward the error message if check fails
    }
    blackThreeCount = cards.size();
    isActive = true;
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

// Implementation of BlackThreeMeld::getTotalPoints
int BlackThreeMeld::updateTotalPoints() {
    points = blackThreeCount * Card(Rank::Three, CardColor::BLACK).getPoints();
    return points;
}

