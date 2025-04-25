

#ifndef MELD_HPP
#define MELD_HPP

#include "card.hpp"
#include <expected>
#include <vector>
#include <cereal/types/vector.hpp>
#include <cereal/types/polymorphic.hpp>
#include <cereal/access.hpp>
#include <cassert>


using Status = std::expected<void, std::string>;

constexpr unsigned CANASTA_TYPE_COUNT = 2;
constexpr std::array<const char *, CARD_COLOR_COUNT> canastaTypes = {
    "Natural", "Mixed", // "Wild" can be added later
};

enum class CanastaType
{
    Natural, // Ranks Four → Ace
    Mixed,   // Ranks Four → Ace with wild cards
    // Wild,  // Only wild cards Not supported in this implementation, but can be added
};

// Overload std::to_string for CardType
inline std::string to_string(CanastaType type) {
    return canastaTypes[static_cast<int>(type)];
}

// --- Scoring Constants ---
constexpr int NATURAL_CANASTA_BONUS = 500;
constexpr int MIXED_CANASTA_BONUS = 300;

// Abstract Base Class for All Melds
class BaseMeld {
protected:
    bool isActive;
    int points; // Cached points value
public:
    BaseMeld() : isActive(false), points(0) {}
    virtual ~BaseMeld() = default;

    virtual Status checkInitialization(const std::vector<Card>& cards) const = 0;
    virtual void initialize(const std::vector<Card>& cards) = 0;
    virtual Status checkCardsAddition(const std::vector<Card>& cards) const = 0;
    virtual void addCards(const std::vector<Card>& cards) = 0;
    virtual int getPoints() const = 0; // Get the cached points
    virtual void updatePoints() = 0;   // Update the cached points
    bool isInitialized() const { return isActive; }

    virtual bool isCanastaMeld() const { return false; } // Default: Not a canasta
    virtual std::optional<CanastaType> getCanastaType() const { return std::nullopt; } // Default: No canasta type

    // Correctly templated serialization method
    template <class Archive>
    void serialize(Archive& archive)
    {
        // Removed updatePoints() call - assume points are updated when state changes
        archive(CEREAL_NVP(isActive), CEREAL_NVP(points));
    }
};

// Templated class for normal melds (Ranks Four → Ace)
template <Rank R>
class Meld final : public BaseMeld {
private:
    bool isCanasta;
    std::vector<Card> naturalCards;
    std::vector<Card> wildCards;

public:
    Meld() : isCanasta(false) {}

    Status checkInitialization(const std::vector<Card>& cards) const override;
    void initialize(const std::vector<Card>& cards) override;
    Status checkCardsAddition(const std::vector<Card>& cards) const override;
    void addCards(const std::vector<Card>& cards) override;
    int getPoints() const override;
    void updatePoints() override;
    bool isCanastaMeld() const override { return isCanasta; }
    std::optional<CanastaType> getCanastaType() const override;
    static bool isCorrectNaturalList(const std::vector<Card>& cards);

    template <class Archive>
    void serialize(Archive& archive) {
        archive(cereal::base_class<BaseMeld>(this), CEREAL_NVP(isCanasta), CEREAL_NVP(naturalCards), CEREAL_NVP(wildCards));
    }
private:
    void updateCanastaStatus();
    Status validateCards(const std::vector<Card>& cards,
        std::size_t naturalCardCount = 0, std::size_t wildCardCount = 0) const;
};

// Red Three Meld (Special case)
class RedThreeMeld final : public BaseMeld {
private:
    int redThreeCount;

public:
    RedThreeMeld() : redThreeCount(0) {}

    Status checkInitialization(const std::vector<Card>& cards) const override;
    void initialize(const std::vector<Card>& cards) override;
    Status checkCardsAddition(const std::vector<Card>& cards) const override;
    void addCards(const std::vector<Card>& cards) override;
    int getPoints() const override;
    void updatePoints() override;

    template <class Archive>
    void serialize(Archive& archive) {
        archive(cereal::base_class<BaseMeld>(this), CEREAL_NVP(redThreeCount));
    }
private:
    Status validateCards(const std::vector<Card>& cards, std::size_t redThreeCount = 0) const;
};

// Black Three Meld (Special case)
class BlackThreeMeld final : public BaseMeld {
private:
    int blackThreeCount;

public:
    BlackThreeMeld() : blackThreeCount(0) {}
    
    Status checkInitialization(const std::vector<Card>& cards) const override;
    void initialize(const std::vector<Card>& cards) override;
    Status checkCardsAddition(const std::vector<Card>& cards) const override;
    void addCards(const std::vector<Card>& cards) override;
    int getPoints() const override;
    void updatePoints() override;

    template <class Archive>
    void serialize(Archive& archive) {
        archive(cereal::base_class<BaseMeld>(this), CEREAL_NVP(blackThreeCount));
    }
};


// Register derived classes for polymorphic serialization
CEREAL_REGISTER_TYPE(RedThreeMeld)
CEREAL_REGISTER_TYPE(BlackThreeMeld)
CEREAL_REGISTER_POLYMORPHIC_RELATION(BaseMeld, RedThreeMeld)
CEREAL_REGISTER_POLYMORPHIC_RELATION(BaseMeld, BlackThreeMeld)


// Implementation of Meld<R>::initialize
template <Rank R>
void Meld<R>::initialize(const std::vector<Card>& cards) {
    auto status = checkInitialization(cards)
    assert(status.has_value() && "Meld initialization failed");
    for (const auto& card : cards) {
        if (card.getType() == CardType::Wild) {
            wildCards.push_back(card);
        } else {
            naturalCards.push_back(card);
        }
    }
    isActive = true;
    updateCanastaStatus();
    updatePoints(); // Update points after initialization
    return {};
}

// Implementation of Meld<R>::validateCards
template <Rank R>
Status Meld<R>::validateCards(const std::vector<Card>& cards,
    std::size_t naturalCardCount, std::size_t wildCardCount) const {
    std::size_t naturalCount = naturalCardCount;
    std::size_t wildCount    = wildCardCount;    
    for (const auto& card : cards) {
        if (card.getType() == CardType::Wild) {
            wildCount++;
        } else if (card.getRank() == R) {
            naturalCount++;
        } else {
            return std::unexpected("Invalid card " + card.toString() + " for this meld");
        }
    }
    if (wildCount > naturalCount) {
        return std::unexpected("Too many wild cards for this meld");
    }
    return {};
}

// Implementation of Meld<R>::checkInitialization
template <Rank R>
Status Meld<R>::checkInitialization(const std::vector<Card>& cards) const {
    if (isActive) {
        return std::unexpected("Meld is already initialized");
    }
    if (cards.size() < 3) {
        return std::unexpected("Meld must contain at least 3 cards");
    }
    return validateCards(cards);
}

// Implementation of Meld<R>::addCard
template <Rank R>
void Meld<R>::addCards(const std::vector<Card>& cards) {
    auto status = checkCardsAddition(cards);
    assert(status.has_value() && "Meld addition failed");
    if (card.getType() == CardType::Wild)
        wildCards.push_back(card);
    else
        naturalCards.push_back(card);
    updateCanastaStatus();
    updatePoints(); // Update points after adding a card
    return {};
}

// Implementation of Meld<R>::checkCardAddition
template <Rank R>
Status Meld<R>::checkCardsAddition(const std::vector<Card>& cards) const {
    if (!isActive) {
        return std::unexpected("Meld is not initialized");
    }
    if (cards.empty()) {
        return std::unexpected("You must add at least 1 card");
    }
    validateCards(cards, naturalCards.size(), wildCards.size());
}

// Implementation of Meld<R>::updateCanastaStatus
template <Rank R>
void Meld<R>::updateCanastaStatus() {
    if (isCanasta) {
        return;
    }
    if (naturalCards.size() + wildCards.size() >= 7) {
        isCanasta = true;
    }
}

// Implementation of Meld<R>::getCanastaType
template <Rank R>
std::optional<CanastaType> Meld<R>::getCanastaType() const {
    if (!isCanasta) {
        return std::nullopt;
    }
    if (wildCards.empty()) {
        return CanastaType::Natural;
    }
    return CanastaType::Mixed;
}

// Implementation of Meld<R>::getPoints
template <Rank R>
int Meld<R>::getPoints() const
{
    // Simply return the cached value
    return points;
}

template <Rank R>
void Meld<R>::updatePoints() {
    int calculatedPoints = 0;
    for (const auto& card : naturalCards) {
        points += card.getPoints();
    }
    for (const auto& card : wildCards) {
        points += card.getPoints();
    }
    if (isCanasta) {
        if (const auto canastaType = getCanastaType(); canastaType.has_value()) {
            if (*canastaType == CanastaType::Natural)
                points += NATURAL_CANASTA_BONUS; // 500 points for a natural canasta
            else if (*canastaType == CanastaType::Mixed)
                points += MIXED_CANASTA_BONUS; // 300 points for a mixed canasta
        }
    }
    points = calculatedPoints;
}

// Implementation of Meld<R>::isCorrectNaturalList
template <Rank R>
bool Meld<R>::isCorrectNaturalList(const std::vector<Card>& cards) {
    for (const auto& card : cards) {
        if (card.getRank() != R) {
            return false;
        }
    }
    return true;
}


#endif //MELD_HPP
