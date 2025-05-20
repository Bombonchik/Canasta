#ifndef MAKE_STATE_HPP
#define MAKE_STATE_HPP

#include "game_state.hpp"    // for ClientGameState
#include "server/round_manager.hpp" // for RoundManager
#include "server/game_manager.hpp"  // for GameManager

/**
 * @brief Creates a ClientGameState object for sending to the client.
 * @param player The player whose state is being created.
 * @param roundManager The current round manager.
 * @param gameManager The current game manager.
 * @param actionDescription Description of the last action taken.
 * @param status Optional status of the last action.
 * @return A ClientGameState object containing the relevant game state.
 */
inline ClientGameState makeClientGameState(
    const Player& player,
    const RoundManager& roundManager,
    const GameManager& gameManager,
    const std::string& actionDescription,
    std::optional<TurnActionStatus> status = std::nullopt
) {
    ClientGameState s;
    s.setDeckState(roundManager.getClientDeck());
    s.setMyPlayerData(player);
    s.setAllPlayersPublicInfo(roundManager.getAllPlayersPublicInfo(player));
    auto team1 = gameManager.getTeam1();
    auto team2 = gameManager.getTeam2();
    auto myTeam = team1.hasPlayer(player) ? team1 : team2;
    auto opponentTeam = team2.hasPlayer(player) ? team1 : team2;
    s.setMyTeamState(roundManager.getTeamStateForTeam(myTeam));
    s.setOpponentTeamState(roundManager.getTeamStateForTeam(opponentTeam));
    s.setMyTeamTotalScore(myTeam.getTotalScore());
    s.setOpponentTeamTotalScore(opponentTeam.getTotalScore());
    s.setIsRoundOver(roundManager.isRoundOver());
    if (roundManager.isRoundOver()) {
        auto scoreMapping = roundManager.calculateScores();
        s.setMyTeamScoreBreakdown(scoreMapping[myTeam.getName()]);
        s.setOpponentTeamScoreBreakdown(scoreMapping[opponentTeam.getName()]);
    }
    s.setIsGameOver(gameManager.isGameOver());
    if (gameManager.isGameOver()) {
        auto winningTeam = gameManager.getWinningTeam();
        if (winningTeam.has_value()) {
            s.setGameOutcome(winningTeam->get().getName() == myTeam.getName() ? 
            ClientGameOutcome::Win : ClientGameOutcome::Lose);
        } else {
            s.setGameOutcome(ClientGameOutcome::Draw);
        }
    }
    s.setLastActionDescription(actionDescription);
    s.setStatus(status);
    return s;
}


#endif // MAKE_STATE_HPP