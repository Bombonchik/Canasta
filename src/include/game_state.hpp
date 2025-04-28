#ifndef GAME_STATE_HPP
#define GAME_STATE_HPP

#include <string>
#include "cereal/cereal.hpp"
#include "cereal/archives/binary.hpp"
#include "player.hpp"           // Includes Hand, Card
#include "team_round_state.hpp" // Includes Meld, ScoreBreakdown
#include "client/client_deck.hpp"      // Public view of deck/discard state
#include "rule_engine.hpp"      // For GameOutcome
#include <vector>
#include <optional>
#include <cereal/types/vector.hpp>
#include <cereal/types/string.hpp>
#include <cereal/types/optional.hpp>

struct GameState {
    int currentPlayer;
    std::string publicMessage;

    template <class Archive>
    void serialize(Archive& archive) {
        archive(currentPlayer, publicMessage);
    }
};

struct PlayerPublicInfo {
    std::string name;
    size_t handCardCount;
    bool isCurrentTurn; // Flag if this player is the current one

    template <class Archive>
    void serialize(Archive& ar) {
        ar(CEREAL_NVP(name), CEREAL_NVP(handCardCount), CEREAL_NVP(isCurrentTurn));
    }
};

struct ClientGameState {
    // Personalized Data
    Player yourPlayerData; // Full Player object (including Hand) for the receiving client

    // Public Data for Others
    std::vector<PlayerPublicInfo> allPlayersPublicInfo; // Includes self and others

    // Team States (Round Specific)
    TeamRoundState team1State; // Full round state for Team 1's melds
    TeamRoundState team2State; // Full round state for Team 2's melds

    // Deck/Discard Public State
    ClientDeck deckState; // Contains top discard, counts, frozen status

    // Overall Game Scores
    int team1TotalScore;
    int team2TotalScore;

    // Game Status
    bool isGameOver = false;
    std::optional<GameOutcome> gameOutcome; // Set only if isGameOver is true

    // Context
    std::string lastActionDescription; // Description of the last successful action

    // Default constructor might be needed for serialization/initialization
    ClientGameState() = default;

    template <class Archive>
    void serialize(Archive& ar) {
        ar(CEREAL_NVP(yourPlayerData), CEREAL_NVP(allPlayersPublicInfo),
            CEREAL_NVP(team1State), CEREAL_NVP(team2State),
            CEREAL_NVP(deckState),
            CEREAL_NVP(team1TotalScore), CEREAL_NVP(team2TotalScore),
            CEREAL_NVP(isGameOver), CEREAL_NVP(gameOutcome),
            CEREAL_NVP(lastActionDescription));
    }
};

#endif //GAME_STATE_HPP
