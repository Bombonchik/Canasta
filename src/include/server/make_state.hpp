#ifndef MAKE_STATE_HPP
#define MAKE_STATE_HPP

#include "game_state.hpp"    // for ClientGameState
#include "server/round_manager.hpp" // for RoundManager
#include "server/game_manager.hpp"  // for GameManager

inline ClientGameState makeClientGameState(
    const Player& player,
    const RoundManager& roundManager,
    const GameManager& gameManager,
    std::string actionDescription,
    std::optional<TurnActionStatus> status = std::nullopt
) {
    ClientGameState s;
    s.deckState = roundManager.getClientDeck();
    s.myPlayerData = player;
    s.allPlayersPublicInfo = roundManager.getAllPlayersPublicInfo(player);
    auto team1 = gameManager.getTeam1();
    auto team2 = gameManager.getTeam2();
    auto myTeam = team1.hasPlayer(player) ? team1 : team2;
    auto opponentTeam = team2.hasPlayer(player) ? team1 : team2;
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
        auto winningTeam = gameManager.getWinningTeam();
        if (winningTeam.has_value()) {
            s.gameOutcome = winningTeam->get().getName() == myTeam.getName() ? 
            ClientGameOutcome::Win : ClientGameOutcome::Lose;
        } else {
            s.gameOutcome = ClientGameOutcome::Draw;
        }
    }
    s.lastActionDescription = actionDescription;
    s.status = status;
    return s;
}


#endif // MAKE_STATE_HPP