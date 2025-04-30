#ifndef MAKE_STATE_HPP
#define MAKE_STATE_HPP

#include "game_state.hpp"    // for ClientGameState
#include "server/round_manager.hpp" // for RoundManager
#include "server/game_manager.hpp"  // for GameManager

inline ClientGameState makeClientGameState(
    const Player& player,
    const RoundManager& roundManager,
    const GameManager& gameManager,
    std::string actionDescription
) {
    ClientGameState s;
    s.deckState = roundManager.getClientDeck();
    s.myPlayerData = player;
    s.allPlayersPublicInfo = roundManager.getAllPlayersPublicInfo();
    auto team1 = gameManager.getTeam1();
    auto team2 = gameManager.getTeam2();
    auto myTeam = team1.hasPlayer(player) ? team1 : team2;
    auto opponentTeam = team2.hasPlayer(player) ? team2 : team1;
    s.myTeamState = roundManager.getTeamStateForTeam(myTeam);
    s.opponentTeamState = roundManager.getTeamStateForTeam(opponentTeam);
    s.myTeamTotalScore = myTeam.getTotalScore();
    s.opponentTeamTotalScore = opponentTeam.getTotalScore();
    s.isRoundOver = roundManager.isRoundOver();
    if (s.isRoundOver) {
        auto scoreMapping = roundManager.calculateScores();
        s.myTeamScoreBreakdown = scoreMapping[myTeam.getName()];
        s.opponentTeamScoreBreakdown = scoreMapping[opponentTeam.getName()];
    }
    s.isGameOver = gameManager.isGameOver();
    if (s.isGameOver) {
        s.gameOutcome = gameManager.getGameOutcome();
    }
    s.lastActionDescription = actionDescription;
    return s;
}


#endif // MAKE_STATE_HPP