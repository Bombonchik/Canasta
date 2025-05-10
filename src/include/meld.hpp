

#ifndef MELD_HPP
#define MELD_HPP

#include "card.hpp"
#include <expected>
#include <vector>
#include <cereal/types/vector.hpp>
#include <cereal/types/polymorphic.hpp>
#include <cereal/types/memory.hpp>
#include <cereal/access.hpp>
#include <cassert>
#include <optional>
#include "spdlog/spdlog.h"


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
    bool hasPendingReversible;
public:
    BaseMeld() : isActive(false), points(0), hasPendingReversible(false) {}
    virtual ~BaseMeld() = default;

    virtual Status checkInitialization(const std::vector<Card>& cards) const = 0;
    virtual void initialize(const std::vector<Card>& cards) = 0;
    virtual Status checkCardsAddition(const std::vector<Card>& cards) const = 0;
    virtual void addCards(const std::vector<Card>& cards, bool reversible = false) = 0;
    virtual int getPoints() const = 0; // Get the cached points
    virtual void updatePoints() = 0;   // Update the cached points
    bool isInitialized() const { return isActive; }

    virtual bool isCanastaMeld() const { return false; } // Default: Not a canasta
    virtual std::optional<CanastaType> getCanastaType() const { return std::nullopt; } // Default: No canasta type

    virtual void reset();
    virtual void revertAddCards() = 0; // Revert the last addCards operation, if applicable

    virtual std::vector<Card> getCards() const = 0;
    virtual std::unique_ptr<BaseMeld> clone() const = 0;

    // Correctly templated serialization method
    template <class Archive>
    void serialize(Archive& archive)
    {
        // Removed updatePoints() call - assume points are updated when state changes
        archive(CEREAL_NVP(isActive), CEREAL_NVP(points), CEREAL_NVP(hasPendingReversible));
    }
};

// Templated class for normal melds (Ranks Four → Ace)
template <Rank R>
class Meld final : public BaseMeld {
private:
    bool isCanasta;

    std::vector<Card> naturalCards;
    std::vector<Card> wildCards;

    std::vector<Card> backupNaturalCards;
    std::vector<Card> backupWildCards;

public:
    Meld() : isCanasta(false) {}

    Status checkInitialization(const std::vector<Card>& cards) const override;
    void initialize(const std::vector<Card>& cards) override;
    Status checkCardsAddition(const std::vector<Card>& cards) const override;
    void addCards(const std::vector<Card>& cards, bool reversible) override;
    int getPoints() const override;
    void updatePoints() override;
    bool isCanastaMeld() const override { return isCanasta; }
    std::optional<CanastaType> getCanastaType() const override;
    static bool isCorrectNaturalList(const std::vector<Card>& cards);

    void reset() override;
    void revertAddCards() override;

    std::vector<Card> getCards() const override;
    std::unique_ptr<BaseMeld> clone() const override;

    template <class Archive>
    void serialize(Archive& archive) {
        archive(cereal::base_class<BaseMeld>(this), CEREAL_NVP(isCanasta),
        CEREAL_NVP(naturalCards), CEREAL_NVP(wildCards), CEREAL_NVP(backupNaturalCards),
        CEREAL_NVP(backupWildCards));
    }
private:
    void updateCanastaStatus();
    Status validateCards(const std::vector<Card>& cards,
        std::size_t naturalCardCount = 0, std::size_t wildCardCount = 0) const;
};

// Red Three Meld (Special case)
class RedThreeMeld final : public BaseMeld {
private:
    std::vector<Card> redThreeCards;
    std::vector<Card> backupRedThreeCards;

public:

    Status checkInitialization(const std::vector<Card>& cards) const override;
    void initialize(const std::vector<Card>& cards) override;
    Status checkCardsAddition(const std::vector<Card>& cards) const override;
    void addCards(const std::vector<Card>& cards, bool reversible) override;
    int getPoints() const override;
    void updatePoints() override;

    void reset() override;
    void revertAddCards() override;

    std::vector<Card> getCards() const override;
    std::unique_ptr<BaseMeld> clone() const override;

    template <class Archive>
    void serialize(Archive& archive) {
        archive(cereal::base_class<BaseMeld>(this), CEREAL_NVP(redThreeCards),
        CEREAL_NVP(backupRedThreeCards));
    }
private:
    Status validateCards(const std::vector<Card>& cards, std::size_t redThreeCount = 0) const;
};

// Black Three Meld (Special case)
class BlackThreeMeld final : public BaseMeld {
private:
    std::vector<Card> blackThreeCards;

public:
    Status checkInitialization(const std::vector<Card>& cards) const override;
    void initialize(const std::vector<Card>& cards) override;
    Status checkCardsAddition(const std::vector<Card>& cards) const override;
    void addCards(const std::vector<Card>& cards, bool reversible) override;
    int getPoints() const override;
    void updatePoints() override;

    void reset() override;
    void revertAddCards() override;

    std::vector<Card> getCards() const override;
    std::unique_ptr<BaseMeld> clone() const override;

    template <class Archive>
    void serialize(Archive& archive) {
        archive(cereal::base_class<BaseMeld>(this), CEREAL_NVP(blackThreeCards));
    }
};


// Register derived classes for polymorphic serialization
CEREAL_REGISTER_TYPE(RedThreeMeld)
CEREAL_REGISTER_TYPE(BlackThreeMeld)
CEREAL_REGISTER_POLYMORPHIC_RELATION(BaseMeld, RedThreeMeld)
CEREAL_REGISTER_POLYMORPHIC_RELATION(BaseMeld, BlackThreeMeld)

// Register specific instantiations of Meld<R>
CEREAL_REGISTER_TYPE(Meld<Rank::Four>)
CEREAL_REGISTER_TYPE(Meld<Rank::Five>)
CEREAL_REGISTER_TYPE(Meld<Rank::Six>)
CEREAL_REGISTER_TYPE(Meld<Rank::Seven>)
CEREAL_REGISTER_TYPE(Meld<Rank::Eight>)
CEREAL_REGISTER_TYPE(Meld<Rank::Nine>)
CEREAL_REGISTER_TYPE(Meld<Rank::Ten>)
CEREAL_REGISTER_TYPE(Meld<Rank::Jack>)
CEREAL_REGISTER_TYPE(Meld<Rank::Queen>)
CEREAL_REGISTER_TYPE(Meld<Rank::King>)
CEREAL_REGISTER_TYPE(Meld<Rank::Ace>)

// Register the polymorphic relationship for each instantiation
CEREAL_REGISTER_POLYMORPHIC_RELATION(BaseMeld, Meld<Rank::Four>)
CEREAL_REGISTER_POLYMORPHIC_RELATION(BaseMeld, Meld<Rank::Five>)
CEREAL_REGISTER_POLYMORPHIC_RELATION(BaseMeld, Meld<Rank::Six>)
CEREAL_REGISTER_POLYMORPHIC_RELATION(BaseMeld, Meld<Rank::Seven>)
CEREAL_REGISTER_POLYMORPHIC_RELATION(BaseMeld, Meld<Rank::Eight>)
CEREAL_REGISTER_POLYMORPHIC_RELATION(BaseMeld, Meld<Rank::Nine>)
CEREAL_REGISTER_POLYMORPHIC_RELATION(BaseMeld, Meld<Rank::Ten>)
CEREAL_REGISTER_POLYMORPHIC_RELATION(BaseMeld, Meld<Rank::Jack>)
CEREAL_REGISTER_POLYMORPHIC_RELATION(BaseMeld, Meld<Rank::Queen>)
CEREAL_REGISTER_POLYMORPHIC_RELATION(BaseMeld, Meld<Rank::King>)
CEREAL_REGISTER_POLYMORPHIC_RELATION(BaseMeld, Meld<Rank::Ace>)

struct MeldRequest {
    std::vector<Card> cards;
    std::optional<Rank> addToRank;
      // - nullopt = “I want to initialize a NEW meld”
      // - Rank    = “I want to add these cards to the existing meld of rank addToRank”

    // Add Cereal serialize method
    template <class Archive>
    void serialize(Archive& ar) {
        // Need to serialize Rank enum as underlying type (e.g., int)
        // Assuming Rank has a Cereal serialization function defined elsewhere
        ar(CEREAL_NVP(cards), CEREAL_NVP(addToRank));
    }
};


// Implementation of Meld<R>::initialize
template <Rank R>
void Meld<R>::initialize(const std::vector<Card>& cards) {
    Status status = checkInitialization(cards);
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
void Meld<R>::addCards(const std::vector<Card>& cards, bool reversible) {
    auto status = checkCardsAddition(cards);
    assert(status.has_value() && "Meld addition failed");

    if (reversible) {
        backupNaturalCards = naturalCards; // Copy current state
        backupWildCards = wildCards;       // Copy current state
        hasPendingReversible = true;
    } else {
        hasPendingReversible = false; // No reversible action
    }
    for (const auto& card : cards) {
        if (card.getType() == CardType::Wild) {
            wildCards.push_back(card);
        } else {
            naturalCards.push_back(card);
        }
    }
    updateCanastaStatus();
    updatePoints(); // Update points after adding a card
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
    return validateCards(cards, naturalCards.size(), wildCards.size());
}

// Implementation of Meld<R>::updateCanastaStatus
template <Rank R>
void Meld<R>::updateCanastaStatus() {
    isCanasta = naturalCards.size() + wildCards.size() >= 7;
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
        calculatedPoints += card.getPoints();
        spdlog::debug("Adding points for card {}: {}",
            card.toString(), card.getPoints());
    }
    for (const auto& card : wildCards) {
        calculatedPoints += card.getPoints();
        spdlog::debug("Adding points for card {}: {}",
            card.toString(), card.getPoints());
    }
    if (isCanasta) {
        if (const auto canastaType = getCanastaType(); canastaType.has_value()) {
            if (*canastaType == CanastaType::Natural)
                calculatedPoints += NATURAL_CANASTA_BONUS; // 500 points for a natural canasta
            else if (*canastaType == CanastaType::Mixed)
                calculatedPoints += MIXED_CANASTA_BONUS; // 300 points for a mixed canasta
        }
    }
    points = calculatedPoints;
    spdlog::debug("Meld points updated: {}", points);
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

// Implementation of Meld<R>::reset
template <Rank R>
void Meld<R>::reset() {
    BaseMeld::reset();
    naturalCards.clear();
    wildCards.clear();
    backupNaturalCards.clear();
    backupWildCards.clear();
    isCanasta = false;
}

// Implementation of Meld<R>::revertAddCards
template <Rank R>
void Meld<R>::revertAddCards() {
    if (!hasPendingReversible) {
        return; // No reversible action to revert
    }
    naturalCards = backupNaturalCards; // Restore previous state
    wildCards = backupWildCards;       // Restore previous state
    hasPendingReversible = false;      // Clear the reversible state
    updateCanastaStatus(); // Update canasta status
    updatePoints(); // Update points after reverting
}

// Implementation of Meld<R>::getCards
template <Rank R>
std::vector<Card> Meld<R>::getCards() const {
    std::vector<Card> allCards;
    allCards.reserve(naturalCards.size() + wildCards.size());
    allCards.insert(allCards.end(), wildCards.begin(), wildCards.end());
    allCards.insert(allCards.end(), naturalCards.begin(), naturalCards.end());
    return allCards;
}

// Implementation of Meld<R>::clone
template <Rank R>
std::unique_ptr<BaseMeld> Meld<R>::clone() const {
    return std::make_unique<Meld<R>>(*this);
}


#endif //MELD_HPP
