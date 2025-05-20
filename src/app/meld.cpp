
#include "meld.hpp"

void BaseMeld::reset() {
    isActive = false;
    points = 0;
    hasPendingReversible = false;
}

// Implementation of RedThreeMeld::initialize
void RedThreeMeld::initialize(const std::vector<Card>& cards) {
    auto status = checkInitialization(cards);
    assert(status.has_value() && "Red Three Meld initialization failed");
    redThreeCards = cards;
    isActive = true;
    updatePoints(); // Update points after initialization
}

// Implementation of RedThreeMeld::validateCards
Status RedThreeMeld::validateCards(const std::vector<Card>& cards, std::size_t redThreeCount) const {
    if (cards.size() + redThreeCount > MAX_SPECIAL_MELD_SIZE) {
        return std::unexpected("Red Three Meld can contain at most " + std::to_string(MAX_SPECIAL_MELD_SIZE) + " cards");
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
void RedThreeMeld::addCards(const std::vector<Card>& cards, bool reversible) {
    auto status = checkCardsAddition(cards);
    assert(status.has_value() && "Red Three Meld addition failed");
    if (reversible) {
        backupRedThreeCards = redThreeCards; // Backup current cards
        hasPendingReversible = true; // Mark as reversible
    } else {
        hasPendingReversible = false; // No reversible action
    }
    redThreeCards.insert(redThreeCards.end(), cards.begin(), cards.end());
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
    return validateCards(cards, redThreeCards.size());
}

// Implementation of RedThreeMeld::getPoints
int RedThreeMeld::getPoints() const {
    // Simply return the cached value
    return points;
}

// Implementation of RedThreeMeld::updatePoints
void RedThreeMeld::updatePoints() {
    const int redThreePointsValue = Card(Rank::Three, CardColor::RED).getPoints();
    int calculatedPoints = redThreeCards.size() * redThreePointsValue;
    if (redThreeCards.size() == MAX_SPECIAL_MELD_SIZE) {
        calculatedPoints *= 2; // double points for a complete Red Three Meld
    }
    points = calculatedPoints; // Update the member variable
}


// Implementation of BlackThreeMeld::initialize
void BlackThreeMeld::initialize(const std::vector<Card>& cards) {
    auto status = checkInitialization(cards);
    assert(status.has_value() && "Black Three Meld initialization failed");
    blackThreeCards = cards;
    isActive = true;
    updatePoints(); // Update points after initialization
}

// Implementation of BlackThreeMeld::checkInitialization
Status BlackThreeMeld::checkInitialization(const std::vector<Card>& cards) const {
    if (isActive) {
        return std::unexpected("Black Three Meld is already initialized");
    }
    if (cards.size() < MIN_MELD_SIZE) {
        return std::unexpected("Black Three Meld must contain at least " + std::to_string(MIN_MELD_SIZE) + " cards");
    }
    if (cards.size() > MAX_SPECIAL_MELD_SIZE) {
        return std::unexpected("Black Three Meld can contain at most " + std::to_string(MAX_SPECIAL_MELD_SIZE) + " cards");
    }
    for (const auto& card : cards) {
        if (card.getType() != CardType::BlackThree) {
            return std::unexpected("Invalid card " + card.toString() + " for Black Three Meld");
        }
    }
    return {};
}

// Implementation of BlackThreeMeld::addCard
void BlackThreeMeld::addCards(const std::vector<Card>& cards, bool reversible) {
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
    points = blackThreeCards.size() * Card(Rank::Three, CardColor::BLACK).getPoints();
}

void RedThreeMeld::reset() {
    BaseMeld::reset();
    redThreeCards.clear();
    backupRedThreeCards.clear();
}

void RedThreeMeld::revertAddCards() {
    if (!hasPendingReversible) {
        return; // No reversible action
    }
    redThreeCards = backupRedThreeCards; // Restore the count
    hasPendingReversible = false; // Clear the reversible state
    updatePoints(); // Update points after reverting
}

void BlackThreeMeld::reset() {
    BaseMeld::reset();
    blackThreeCards.clear();
}

void BlackThreeMeld::revertAddCards() {
    throw std::logic_error("BlackThreeMeld: revertAddCards() unsupported");
}

std::vector<Card> RedThreeMeld::getCards() const {
    return redThreeCards;
}

std::vector<Card> BlackThreeMeld::getCards() const {
    return blackThreeCards;
}

std::unique_ptr<BaseMeld> RedThreeMeld::clone() const {
    return std::make_unique<RedThreeMeld>(*this);
}

std::unique_ptr<BaseMeld> BlackThreeMeld::clone() const {
    return std::make_unique<BlackThreeMeld>(*this);
}