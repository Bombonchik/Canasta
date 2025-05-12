#ifndef GAME_STATE_HPP
#define GAME_STATE_HPP

#include <string>
#include "cereal/cereal.hpp"
#include "cereal/archives/binary.hpp"
#include "player_public_info.hpp"
#include "player.hpp"           // Includes Hand, Card
#include "team_round_state.hpp" // Includes Meld, ScoreBreakdown
#include "client_deck.hpp"      // Public view of deck/discard state
#include "rule_engine.hpp"      // For GameOutcome
#include "score_details.hpp"     // For ScoreBreakdown
#include <vector>
#include <optional>
#include <cereal/types/vector.hpp>
#include <cereal/types/string.hpp>
#include <cereal/types/optional.hpp>

enum class ClientGameOutcome {
    Win,
    Lose,
    Draw
};

struct ClientGameState {
    // Deck/Discard Public State
    ClientDeck deckState; // Contains top discard, counts, frozen status
    // Personalized Data
    Player myPlayerData; // Full Player object (including Hand) for the receiving client

    // Public Data for Others
    std::vector<PlayerPublicInfo> allPlayersPublicInfo; // Includes self and others

    // Team States (Round Specific)
    TeamRoundState myTeamState; // Full round state for player's team
    TeamRoundState opponentTeamState; // Full round state for opponent's team

    // Overall Game Scores
    int myTeamTotalScore;
    int opponentTeamTotalScore;

    bool isRoundOver = false; // Flag for round end
    std::optional<ScoreBreakdown> myTeamScoreBreakdown; // Optional, only in round over
    std::optional<ScoreBreakdown> opponentTeamScoreBreakdown; // Optional, only in round over

    // Game Status
    bool isGameOver = false;
    std::optional<ClientGameOutcome> gameOutcome; // Set only if isGameOver is true

    // Context
    std::string lastActionDescription; // Description of the last successful action
    std::optional<TurnActionStatus> status;

    // Default constructor might be needed for serialization/initialization
    ClientGameState() = default;

    template <class Archive>
    void serialize(Archive& ar) {
        ar(CEREAL_NVP(deckState), CEREAL_NVP(myPlayerData),
            CEREAL_NVP(allPlayersPublicInfo), 
            CEREAL_NVP(myTeamState), CEREAL_NVP(opponentTeamState),
            CEREAL_NVP(myTeamTotalScore), CEREAL_NVP(opponentTeamTotalScore),
            CEREAL_NVP(isRoundOver),
            CEREAL_NVP(myTeamScoreBreakdown), CEREAL_NVP(opponentTeamScoreBreakdown),
            CEREAL_NVP(isGameOver), CEREAL_NVP(gameOutcome),
            CEREAL_NVP(lastActionDescription));
    }
};

#endif //GAME_STATE_HPP
