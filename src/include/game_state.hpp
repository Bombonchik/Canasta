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

/**
 * @enum ClientGameOutcome
 * @brief Enum representing the outcome of the game for the client.
 */
enum class ClientGameOutcome {
    Win,
    Lose,
    Draw
};

/**
 * @class ClientGameState
 * @brief Class representing the game state visible to a client.
 * @details This class is used to send game state information to clients.
 */
class ClientGameState {
private:
    /**
     * @brief The state of the main deck and discard pile.
     * @details This includes the size of the main deck, the top card of the discard pile,
     *          the size of the discard pile, and whether the discard pile is frozen.
     */
    ClientDeck deckState;
    /**
     * @brief The player's data, full Player object (including Hand).
     */
    Player myPlayerData;

    /**
     * @brief Public information about all players in the game.
     */
    std::vector<PlayerPublicInfo> allPlayersPublicInfo; // Includes self and others

    // Team States (Round Specific)
    /**
     * @brief The full round state for the player's team.
     */
    TeamRoundState myTeamState;
    /**
     * @brief The full round state for the opponent's team.
     */
    TeamRoundState opponentTeamState;

    // Overall Game Scores
    int myTeamTotalScore;       ///< Total score of the player's team
    int opponentTeamTotalScore; ///< Total score of the opponent's team

    bool isRoundOver = false; ///< Flag for round end
    std::optional<ScoreBreakdown> myTeamScoreBreakdown; ///< Score breakdown for the player's team (only in round over)
    std::optional<ScoreBreakdown> opponentTeamScoreBreakdown; ///< Score breakdown for the opponent's team (only in round over)

    // Game Status
    bool isGameOver = false; ///< Flag for game end
    std::optional<ClientGameOutcome> gameOutcome; ///< Outcome of the game (only if game is over)

    // Context
    std::string lastActionDescription; ///< Description of the last successful action
    std::optional<TurnActionStatus> status; ///< Status of the last action (if applicable)

public:
    /**
     * @brief Default constructor for ClientGameState only for serialization purposes.
     */
    ClientGameState() = default;

    // Setters
    void setDeckState(const ClientDeck& deck) { deckState = deck; }
    void setMyPlayerData(const Player& player) { myPlayerData = player; }
    void setAllPlayersPublicInfo(const std::vector<PlayerPublicInfo>& players) { allPlayersPublicInfo = players; }
    void setMyTeamState(const TeamRoundState& teamState) { myTeamState = teamState.clone(); }
    void setOpponentTeamState(const TeamRoundState& teamState) { opponentTeamState = teamState.clone(); }
    void setMyTeamTotalScore(int score) { myTeamTotalScore = score; }
    void setOpponentTeamTotalScore(int score) { opponentTeamTotalScore = score; }
    void setIsRoundOver(bool roundOver) { isRoundOver = roundOver; }
    void setMyTeamScoreBreakdown(const ScoreBreakdown& breakdown) { myTeamScoreBreakdown = breakdown; }
    void setOpponentTeamScoreBreakdown(const ScoreBreakdown& breakdown) { opponentTeamScoreBreakdown = breakdown; }
    void setIsGameOver(bool gameOver) { isGameOver = gameOver; }
    void setGameOutcome(ClientGameOutcome outcome) { gameOutcome = outcome; }
    void setLastActionDescription(const std::string& description) { lastActionDescription = description; }
    void setStatus(std::optional<TurnActionStatus> actionStatus) { status = actionStatus; }

    // Getters
    const ClientDeck& getDeckState() const { return deckState; }
    const Player& getMyPlayerData() const { return myPlayerData; }
    const std::vector<PlayerPublicInfo>& getAllPlayersPublicInfo() const { return allPlayersPublicInfo; }
    const TeamRoundState& getMyTeamState() const { return myTeamState; }
    const TeamRoundState& getOpponentTeamState() const { return opponentTeamState; }
    int getMyTeamTotalScore() const { return myTeamTotalScore; }
    int getOpponentTeamTotalScore() const { return opponentTeamTotalScore; }
    bool getIsRoundOver() const { return isRoundOver; }
    const std::optional<ScoreBreakdown>& getMyTeamScoreBreakdown() const { return myTeamScoreBreakdown; }
    const std::optional<ScoreBreakdown>& getOpponentTeamScoreBreakdown() const { return opponentTeamScoreBreakdown; }
    bool getIsGameOver() const { return isGameOver; }
    const std::optional<ClientGameOutcome>& getGameOutcome() const { return gameOutcome; }
    const std::string& getLastActionDescription() const { return lastActionDescription; }
    const std::optional<TurnActionStatus>& getStatus() const { return status; }

    /**
     * @brief Serialize the ClientGameState object using Cereal.
     */
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
