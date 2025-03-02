

#ifndef MELD_HPP
#define MELD_HPP

#include "card.hpp"
#include <expected>
#include <vector>
#include <cereal/types/vector.hpp>
#include <cereal/types/polymorphic.hpp>
#include <cereal/access.hpp>


using Status = std::expected<void, std::string>;

constexpr unsigned CANASTA_TYPE_COUNT = 2;
constexpr std::array<const char*, CARD_COLOR_COUNT> canastaTypes = {
    "Natural", "Mixed", // "Wild" can be added later
};

enum class CanastaType {
    Natural,  // Ranks Four → Ace
    Mixed,   // Ranks Four → Ace with wild cards
    //Wild,  // Only wild cards Not supported in this implementation, but can be added
};

// Overload std::to_string for CardType
inline std::string to_string(CanastaType type) {
    return canastaTypes[static_cast<int>(type)];
}

// Abstract Base Class for All Melds
class BaseMeld {
protected:
    bool isActive;
    int points;
public:
    BaseMeld() : isActive(false), points(0) {}
    virtual ~BaseMeld() = default;

    virtual Status initialize(const std::vector<Card>& cards) = 0;
    virtual Status addCard(const Card& card) = 0;
    virtual int updateTotalPoints() = 0;
    bool isInitialized() const { return isActive; }

    // Correctly templated serialization method
    template <class Archive>
    void serialize(Archive& archive) {
        updateTotalPoints();
        archive(isActive, points);
    }
protected:
    virtual Status checkInitialization(const std::vector<Card>& cards) const = 0;
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

    Status initialize(const std::vector<Card>& cards) override;
    Status addCard(const Card& card) override;
    int updateTotalPoints() override;
    bool isCanastaMeld() const { return isCanasta; }
    std::expected<CanastaType, std::string> getCanastaType() const;
    static bool isCorrectNaturalList(const std::vector<Card>& cards);

    template <class Archive>
    void serialize(Archive& archive) {
        archive(cereal::base_class<BaseMeld>(this), isCanasta, naturalCards, wildCards);
    }
protected:
    Status checkInitialization(const std::vector<Card>& cards) const override;
private:
    Status checkCardAddition(const Card& card) const;
    void updateCanastaStatus();
};

// Red Three Meld (Special case)
class RedThreeMeld final : public BaseMeld {
private:
    int redThreeCount;

public:
    RedThreeMeld() : redThreeCount(0) {}

    Status initialize(const std::vector<Card>& cards) override;
    Status addCard(const Card& card) override;
    int updateTotalPoints() override;

    template <class Archive>
    void serialize(Archive& archive) {
        archive(cereal::base_class<BaseMeld>(this), redThreeCount);
    }
protected:
    Status checkInitialization(const std::vector<Card>& cards) const override;
private:
    Status checkCardAddition(const Card& card) const;
};

// Black Three Meld (Special case)
class BlackThreeMeld final : public BaseMeld {
private:
    int blackThreeCount;

public:
    BlackThreeMeld() : blackThreeCount(0) {}

    Status initialize(const std::vector<Card>& cards) override;
    Status addCard(const Card& card) override;
    int updateTotalPoints() override;

    template <class Archive>
    void serialize(Archive& archive) {
        archive(cereal::base_class<BaseMeld>(this), blackThreeCount);
    }
protected:
    Status checkInitialization(const std::vector<Card>& cards) const override;
};


// Register derived classes for polymorphic serialization
CEREAL_REGISTER_TYPE(RedThreeMeld)
CEREAL_REGISTER_TYPE(BlackThreeMeld)
CEREAL_REGISTER_POLYMORPHIC_RELATION(BaseMeld, RedThreeMeld)
CEREAL_REGISTER_POLYMORPHIC_RELATION(BaseMeld, BlackThreeMeld)

// Implementation of Meld<R>::initialize
template <Rank R>
Status Meld<R>::initialize(const std::vector<Card>& cards) {
    if (auto status = checkInitialization(cards); !status.has_value()) {
        return status; // Forward the error message if check fails
    }
    for (const auto& card : cards) {
        if (card.getType() == CardType::Wild) {
            wildCards.push_back(card);
        } else {
            naturalCards.push_back(card);
        }
    }
    isActive = true;
    updateCanastaStatus();
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
    int naturalCardCount = 0;
    int wildCardCount = 0;
    for (const auto& card : cards) {
        if (card.getType() == CardType::Wild) {
            wildCardCount++;
        } else if (card.getRank() == R) {
            naturalCardCount++;
        } else {
            return std::unexpected("Invalid card " + card.toString() + " for this meld");
        }
    }
    if (wildCardCount > naturalCardCount) {
        return std::unexpected("Too many wild cards for this meld");
    }
    return {};
}

// Implementation of Meld<R>::addCard
template <Rank R>
Status Meld<R>::addCard(const Card& card) {
    if (auto status = checkCardAddition(card); !status.has_value()) {
        return status; // Forward the error message if check fails
    }
    if (card.getType() == CardType::Wild) {
        wildCards.push_back(card);
    } else {
        naturalCards.push_back(card);
    }
    updateCanastaStatus();
    return {};
}

// Implementation of Meld<R>::checkCardAddition
template <Rank R>
Status Meld<R>::checkCardAddition(const Card& card) const {
    if (!isActive) {
        return std::unexpected("Meld is not initialized");
    }
    if (card.getType() == CardType::Wild) {
        if (wildCards.size() + 1 > naturalCards.size()) {
            return std::unexpected("Too many wild cards for this meld");
        }
        return {};
    }
    if (card.getRank() != R) {
        return std::unexpected("Invalid card for this meld");
    }
    return {};
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
std::expected<CanastaType, std::string> Meld<R>::getCanastaType() const {
    if (!isCanasta) {
        return std::unexpected("Meld is not a canasta");
    }
    if (wildCards.empty()) {
        return CanastaType::Natural;
    }
    return CanastaType::Mixed;
}

// Implementation of Meld<R>::getTotalPoints
template <Rank R>
int Meld<R>::updateTotalPoints() {
    points = 0;
    for (const auto& card : naturalCards) {
        points += card.getPoints();
    }
    for (const auto& card : wildCards) {
        points += card.getPoints();
    }
    if (isCanasta) {
        if (const auto canastaType = getCanastaType(); canastaType == CanastaType::Natural) {
            points += 500; // 500 points for a natural canasta
        } else if (canastaType == CanastaType::Mixed) {
            points += 300; // 300 points for a mixed canasta
        }
    }
    return points;
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
