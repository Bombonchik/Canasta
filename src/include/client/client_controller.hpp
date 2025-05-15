#ifndef CLIENT_CONTROLLER_HPP
#define CLIENT_CONTROLLER_HPP

#include <string>
#include <functional>
#include <vector>
#include "client_network.hpp"
#include "game_view.hpp"
#include "game_state.hpp" // For ClientGameState, ActionError, MeldRequest, Card

// Forward declarations
class ClientNetwork;
class GameView;

/**
 * @enum ActionAttemptStatus
 * @brief Represents the status of an attempted player action, tracked internally by ClientController.
 */
enum class ActionAttemptStatus {
    NotAttempted,   ///< The action has not yet been tried in the current context/turn.
    Attempting,     ///< The action has been initiated with the server, awaiting response.
    Succeeded      ///< The server confirmed the action was successful.
};

class ClientController {
public:
    /**
     * @brief Constructor for ClientController.
     * @param clientNetwork A shared pointer to the ClientNetwork instance.
     * @param gameView A reference to the GameView instance for user interaction.
     */
    ClientController(std::shared_ptr<ClientNetwork> clientNetwork, GameView& gameView);

    /**
     * @brief Initiates the connection process to the server.
     * Prompts the user for their name via GameView, then calls ClientNetwork::connect.
     * Sets up the necessary callbacks on ClientNetwork.
     * @param host The server host address.
     * @param port The server port.
     */
    void connect(const std::string& host, const std::string& port);

private:
    std::shared_ptr<ClientNetwork> network; /// Pointer to the ClientNetwork instance for network operations
    GameView& view;                         ///< Reference to the GameView instance for user interaction
    std::string localPlayerName;            ///< To store the player's name after successful input
    BoardState currentBoardState;           ///< To keep track of the current board state

    // --- Internal Action State Tracking ---
    // These statuses help the controller decide what to do next based on the game state
    // and the results of previous actions.
    ActionAttemptStatus drawDeckAttemptStatus;
    ActionAttemptStatus takeDiscardPileAttemptStatus;
    ActionAttemptStatus meldAttemptStatus;
    ActionAttemptStatus discardAttemptStatus;

    /// Helper to reset action statuses, e.g., at the start of a new turn.
    void resetTurnActionStatuses();

    // --- Callback Handlers from ClientNetwork ---
    // These methods are the entry points for server-driven events.
    void handleGameStateUpdate(const ClientGameState& gameState);
    void handleActionError(const ActionError& error);
    void handleLoginSuccess();
    void handleLoginFailure(const std::string& reason);
    void handleDisconnect();

    // --- Internal Game Logic ---
    /// Process the current player's turn.
    void processPlayerTurn(std::optional<const std::string> message = std::nullopt); 
    
    // Methods to prompt user for actions via GameView and then call ClientNetwork
    /**
     * @brief Prompts the user to choose between drawing a card from the deck or taking the discard pile.
     */
    void promptAndProcessDrawCardOrTakeDiscardPile
    (std::optional<const std::string> message = std::nullopt);
    /**
     * @brief Processes the turn flow after drawing a card from the deck.
     */
    void processAfterDrawing(std::optional<const std::string> message = std::nullopt);
    /**
     * @brief Processes the turn flow after taking the discard pile.
     */
    void processAfterTakingDiscardPile(std::optional<const std::string> message = std::nullopt);
    /**
     * @brief Processes the turn flow after melding.
     */
    void processAfterMelding(std::optional<const std::string> message = std::nullopt);
    /**
     * @brief Processes the melding action.
     */
    void processMelding(ActionAttemptStatus& previousAttemptStatus);
    /**
     * @brief Processes the revert action.
     */
    void processRevert();
        /**
     * @brief Processes the discard action.
     */
    void processDiscard();

    /// Internal helper to setup callbacks on ClientNetwork
    void setupNetworkCallbacks();

    /**
     * @brief Gets the meld views from the team round state.
     */
    std::vector<MeldView> getMeldViewsFromTeamRoundState(const TeamRoundState& teamRoundState) const;
    /**
     * @brief Gets the board state from the game state.
     */
    BoardState getBoardState(const ClientGameState& gameState) const;
    /**
     * @brief Gets the score state from the game state.
     */
    ScoreState getScoreState(const ClientGameState& gameState) const;
};

#endif // CLIENT_CONTROLLER_HPP