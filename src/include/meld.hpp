

#ifndef MELD_HPP
#define MELD_HPP

#include "card.hpp"
#include <expected>
#include <vector>
#include <cereal/types/vector.hpp>
#include <cereal/types/polymorphic.hpp>
#include <cereal/access.hpp>

using Status = std::expected<void, std::string>;

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
    virtual int getTotalPoints() const = 0;
    bool isInitialized() const { return isActive; }

    // Correctly templated serialization method
    template <class Archive>
    void serialize(Archive& archive) {
        archive(isActive, points);
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

    Status initialize(const std::vector<Card>& cards) override;
    Status addCard(const Card& card) override;
    int getTotalPoints() const override;
    bool isCanastaMeld() const { return isCanasta; }
    bool isCorrectNaturalList(const std::vector<Card>& cards) const;

    template <class Archive>
    void serialize(Archive& archive) {
        archive(cereal::base_class<BaseMeld>(this), isCanasta, naturalCards, wildCards);
    }
};

// Red Three Meld (Special case)
class RedThreeMeld final : public BaseMeld {
private:
    int redThreeCount;

public:
    RedThreeMeld() : redThreeCount(0) {}

    Status initialize(const std::vector<Card>& cards) override;
    Status addCard(const Card& card) override;
    int getTotalPoints() const override;

    template <class Archive>
    void serialize(Archive& archive) {
        archive(cereal::base_class<BaseMeld>(this), redThreeCount);
    }
};

// Black Three Meld (Special case)
class BlackThreeMeld final : public BaseMeld {
private:
    int blackThreeCount;

public:
    BlackThreeMeld() : blackThreeCount(0) {}

    Status initialize(const std::vector<Card>& cards) override;
    Status addCard(const Card& card) override;
    int getTotalPoints() const override;

    template <class Archive>
    void serialize(Archive& archive) {
        archive(cereal::base_class<BaseMeld>(this), blackThreeCount);
    }
};

// Register derived classes for polymorphic serialization
CEREAL_REGISTER_TYPE(RedThreeMeld)
CEREAL_REGISTER_TYPE(BlackThreeMeld)
CEREAL_REGISTER_POLYMORPHIC_RELATION(BaseMeld, RedThreeMeld)
CEREAL_REGISTER_POLYMORPHIC_RELATION(BaseMeld, BlackThreeMeld)

#endif //MELD_HPP
