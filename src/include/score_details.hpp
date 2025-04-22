
#ifndef SCORE_DETAILS_HPP
#define SCORE_DETAILS_HPP

#include <cereal/access.hpp>
#include <cereal/types/string.hpp> // Include if adding player names later

// Holds the breakdown of points scored by a team in a single round,
// excluding the "Going Out" bonus which is added at the Round/Game level.
struct ScoreBreakdown {
    int naturalCanastaBonus = 0;  // Total bonus for Natural Canastas (N * 500)
    int mixedCanastaBonus = 0;    // Total bonus for Mixed Canastas (M * 300)
    int meldedCardsPoints = 0;      // Sum of points from individual cards in melds (excluding Red Threes)
    int redThreeBonusPoints = 0; // Bonus for Red Threes (100 each, 800 total for 4)
    int handPenaltyPoints = 0;   // Penalty for cards left in hand (negative value)

    ScoreBreakdown() = default;

    // Calculate total points from this breakdown (excluding Going Out bonus)
    int calculateSubTotal() const {
        return naturalCanastaBonus + mixedCanastaBonus + meldedCardsPoints + redThreeBonusPoints + handPenaltyPoints;
    }

    template <class Archive>
    void serialize(Archive& archive) {
        archive(CEREAL_NVP(naturalCanastaBonus), CEREAL_NVP(mixedCanastaBonus),
                CEREAL_NVP(meldedCardsPoints), CEREAL_NVP(redThreeBonusPoints),
                CEREAL_NVP(handPenaltyPoints));
    }
};

#endif // SCORE_DETAILS_HPP