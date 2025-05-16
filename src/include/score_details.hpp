
#ifndef SCORE_DETAILS_HPP
#define SCORE_DETAILS_HPP

#include <cereal/access.hpp>
#include <cereal/types/string.hpp> // Include if adding player names later

// Holds the breakdown of points scored by a team in a single round
/**
 * @class ScoreBreakdown
 * @brief Class representing the breakdown of points scored by a team in a single round.
 */
class ScoreBreakdown {
private:
    /**
     * @brief Total bonus for Natural Canastas (N * 500)
     */
    int naturalCanastaBonus = 0;
    /**
     * @brief Total bonus for Mixed Canastas (M * 300)
     */
    int mixedCanastaBonus = 0;
    /**
     * @brief Total points from individual cards in melds (excluding Red Threes)
     */
    int meldedCardsPoints = 0;
    /**
     * @brief Total bonus for Red Threes (100 each, 800 total for 4)
     * @details Could be negative if team has no melds
     */
    int redThreeBonusPoints = 0;
    /**
     * @brief Total penalty points for cards left in hand (negative value)
     */
    int handPenaltyPoints = 0;
    /**
     * @brief Bonus for going out (100 points if going out)
     */
    int goingOutBonus = 0;
    
public:

    ScoreBreakdown() = default;

    // Getters
    int getNaturalCanastaBonus() const { return naturalCanastaBonus; }
    int getMixedCanastaBonus() const { return mixedCanastaBonus; }
    int getMeldedCardsPoints() const { return meldedCardsPoints; }
    int getRedThreeBonusPoints() const { return redThreeBonusPoints; }
    int getHandPenaltyPoints() const { return handPenaltyPoints; }
    int getGoingOutBonus() const { return goingOutBonus; }

    // Setters
    void setNaturalCanastaBonus(int bonus) { naturalCanastaBonus = bonus; }
    void setMixedCanastaBonus(int bonus) { mixedCanastaBonus = bonus; }
    void setMeldedCardsPoints(int points) { meldedCardsPoints = points; }
    void setRedThreeBonusPoints(int points) { redThreeBonusPoints = points; }
    void setHandPenaltyPoints(int points) { handPenaltyPoints = points; }
    void setGoingOutBonus(int bonus) { goingOutBonus = bonus; }

    /**
     * @brief Calculate the total score based on the breakdown.
     */
    int calculateTotal() const {
        return naturalCanastaBonus + mixedCanastaBonus + meldedCardsPoints 
        + redThreeBonusPoints + handPenaltyPoints + goingOutBonus;
    }

    /**
     * @brief Serialize the ScoreBreakdown using Cereal.
     * @param archive The archive to serialize to.
     */
    template <class Archive>
    void serialize(Archive& archive) {
        archive(CEREAL_NVP(naturalCanastaBonus), CEREAL_NVP(mixedCanastaBonus),
                CEREAL_NVP(meldedCardsPoints), CEREAL_NVP(redThreeBonusPoints),
                CEREAL_NVP(handPenaltyPoints), CEREAL_NVP(goingOutBonus));
    }
};

#endif // SCORE_DETAILS_HPP