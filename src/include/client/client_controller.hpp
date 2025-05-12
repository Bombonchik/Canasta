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
 * @brief Represents the status of an attempted player action, tracked internally by ClientController.
 */
enum class ActionAttemptStatus {
    NotAttempted,   // The action has not yet been tried in the current context/turn.
    Attempting,     // The action has been initiated with the server, awaiting response.
    Succeeded      // The server confirmed the action was successful.
};

class ClientController {
public:
    ClientController(std::shared_ptr<ClientNetwork> clientNetwork, GameView& gameView);

    /**
     * @brief Initiates the connection process to the server.
     * Prompts the user for their name via GameView, then calls ClientNetwork::connect.
     * Sets up the necessary callbacks on ClientNetwork.
     * @param host The server host address.
     * @param port The server port.
     */
    void connect(const std::string& host, const std::string& port);

    // The main logic for handling game flow, player input, and server communication
    // will be driven by the callbacks from ClientNetwork, primarily handleGameStateUpdate.

private:
    std::shared_ptr<ClientNetwork> network;
    GameView& view;
    std::string localPlayerName; // To store the player's name after successful input
    BoardState currentBoardState; // To keep track of the current board state

    // --- Internal Action State Tracking ---
    // These statuses help the controller decide what to do next based on the game state
    // and the results of previous actions.
    ActionAttemptStatus drawDeckAttemptStatus;
    ActionAttemptStatus takeDiscardPileAttemptStatus;
    ActionAttemptStatus meldAttemptStatus;
    ActionAttemptStatus discardAttemptStatus;

    // Helper to reset action statuses, e.g., at the start of a new turn or after a revert.
    void resetTurnActionStatuses();

    // --- Callback Handlers from ClientNetwork ---
    // These methods are the entry points for server-driven events.
    void handleGameStateUpdate(const ClientGameState& gameState);
    void handleActionError(const ActionError& error);
    void handleLoginSuccess();
    void handleLoginFailure(const std::string& reason);
    void handleDisconnect();

    // --- Internal Game Logic ---
    // Called by handleGameStateUpdate to process the current player's turn.
    void processPlayerTurn(std::optional<const std::string> message = std::nullopt); 
    
    // Methods to prompt user for actions via GameView and then call ClientNetwork
    void promptAndProcessDrawCardOrTakeDiscardPile
    (std::optional<const std::string> message = std::nullopt);
    void processAfterDrawing(std::optional<const std::string> message = std::nullopt);
    void processAfterTakingDiscardPile(std::optional<const std::string> message = std::nullopt);
    void processAfterMelding(std::optional<const std::string> message = std::nullopt);
    void processMelding(ActionAttemptStatus& previousAttemptStatus);
    void processRevert();
    void processDiscard();

    // Internal helper to setup callbacks on ClientNetwork
    void setupNetworkCallbacks();

    std::vector<MeldView> getMeldViewsFromTeamRoundState(const TeamRoundState& teamRoundState) const;
    BoardState getBoardState(const ClientGameState& gameState) const;
    ScoreState getScoreState(const ClientGameState& gameState) const;
};

#endif // CLIENT_CONTROLLER_HPP