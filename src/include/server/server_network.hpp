#ifndef SERVER_NETWORK_HPP
#define SERVER_NETWORK_HPP

#include <memory>
#include <vector>
#include <string>
#include <set>
#include <deque>
#include <mutex> // For thread safety if needed later, though game logic runs on one strand
#include <functional> // For std::function

#include "game_manager.hpp" // To interact with the game logic
#include "game_state.hpp"   // For ClientGameState
#include "network.hpp"

// Forward declaration
class Session;

// Type alias for shared pointer to Session
using SessionPtr = std::shared_ptr<Session>;

// Type alias for message queue
using MessageQueue = std::deque<std::vector<char>>; // Store serialized messages as byte vectors

/**
 * @brief Handles network connections, message framing, serialization,
 *        and dispatching actions to the GameManager. Manages client sessions.
 */
class ServerNetwork {
public:
    /**
     * @brief Constructor.
     * @param ioContext The ASIO I/O context to run on.
     * @param endpoint The TCP endpoint to listen on (e.g., tcp::v4(), port).
     * @param gameManager Reference to the single GameManager instance.
     */
    ServerNetwork(asio::io_context& ioContext,
                const asio::ip::tcp::endpoint& endpoint,
                GameManager& gameManager);

    /**
     * @brief Starts the server listening for incoming connections.
     */
    void startAccept();

    /**
     * @brief Delivers a serialized message (e.g., game state update) to a specific player/session.
     * @param playerName The name of the player to send the message to.
     * @param message The serialized message data.
     */
    void deliverToOne(const std::string& playerName, const std::vector<char>& message);

    /**
     * @brief Delivers a serialized message to all currently connected sessions.
     * @param message The serialized message data.
     */
    void deliverToAll(const std::vector<char>& message);

    // --- Action Handling (Called by Session) ---
    // These methods will be called by a Session when it receives a complete message.
    // They need to deserialize the action parameters and dispatch to GameManager/RoundManager
    // on the correct strand/thread.

    void handleClientDrawDeck(const std::string& playerName);
    void handleClientTakeDiscardPile(const std::string& playerName);
    void handleClientMeld(const std::string& playerName, const std::vector<MeldRequest>& meldRequests);
    void handleClientDiscard(const std::string& playerName, const Card& cardToDiscard);
    void handleClientRevert(const std::string& playerName);
    // Add handlers for other potential client messages (e.g., connect, disconnect, chat)

private:
    friend class Session; // Allow Session to access private members

    /**
     * @brief Registers a new session and associates it with a player name.
     * @param session The session pointer.
     * @param playerName The name provided by the client upon connection.
     */
    Status join(SessionPtr session, const std::string& playerName);

    /**
     * @brief Removes a session when a client disconnects.
     * @param session The session pointer.
     */
    void leave(SessionPtr session);

    template<typename ActionFn>
    void dispatchAction(const std::string& playerName, ActionFn&& action);

    void sendActionError(const std::string& playerName, const std::string& errorMsg,
        std::optional<TurnActionStatus> status = std::nullopt);

    void broadcastGameState(const std::string& lastActionMsg,
        std::optional<TurnActionStatus> status = std::nullopt);

    // --- Member Variables ---
    asio::io_context& ioContext; // Reference to the main I/O context
    asio::ip::tcp::acceptor acceptor; // Accepts incoming connections
    GameManager& gameManager; // Reference to the game logic orchestrator

    // Use a strand to ensure game logic calls happen sequentially for the single game instance
    asio::strand<asio::io_context::executor_type> gameStrand;

    // Manages active sessions (map player name to session)
    std::map<std::string, SessionPtr> sessions;
    // Mutex for protecting access to sessions map if accessed from multiple threads (acceptor vs session handlers)
    std::mutex sessionsMutex;
};

template<typename ActionFn>
void ServerNetwork::dispatchAction(const std::string& playerName, ActionFn&& action) {
    assert(gameStrand.running_in_this_thread());
    auto *rm = gameManager.getCurrentRoundManager();
    if (!rm || rm->getCurrentPlayer().getName() != playerName) {
        sendActionError(playerName, "Not your turn or round not active.");
        return;
    }
    TurnActionResult result = action(*rm);
    if (result.status < TurnActionStatus::Error_MainDeckEmpty) {
        // success → broadcast the result.message
        broadcastGameState(result.message, result.status);
    } else {
        // failure → send error back to just that player
        sendActionError(playerName, result.message, result.status);
    }
}

//----------------------------------------------------------------------

/**
 * @brief Represents a single client connection session.
 * Handles reading/writing messages for one client.
 */
class Session : public std::enable_shared_from_this<Session> {
public:
    Session(asio::ip::tcp::socket socket, ServerNetwork& serverNetwork, asio::strand<asio::io_context::executor_type>& gameStrand);

    /**
     * @brief Starts the session (begins reading messages).
     */
    void start();

    /**
     * @brief Delivers a pre-serialized message to this client.
     * @param message The serialized message data.
     */
    void deliver(const std::vector<char>& message);

    /**
     * @brief Gets the player name associated with this session.
     * @return Player name string (might be empty initially).
     */
    const std::string& getPlayerName() const;

private:
    /**
     * @brief Initiates an asynchronous read for the message header (size).
     */
    void doReadHeader();

    /**
     * @brief Callback for when the message header has been read.
     */
    void handleReadHeader(const asio::error_code& error, std::size_t /*bytes_transferred*/);

    /**
     * @brief Initiates an asynchronous read for the message body.
     */
    void doReadBody();

    /**
     * @brief Callback for when the message body has been read.
     */
    void handleReadBody(const asio::error_code& error, std::size_t /*bytes_transferred*/);

    /**
     * @brief Initiates an asynchronous write operation.
     */
    void doWrite();

    /**
     * @brief Callback for when a write operation completes.
     */
    void handleWrite(const asio::error_code& error, std::size_t /*bytes_transferred*/);

    /**
     * @brief Parses the received message body and dispatches the action.
     */
    void processMessage();

    void processLoginMessage(ClientMessageType msgType, cereal::BinaryInputArchive& archive);

    void processGameMessage(ClientMessageType msgType, cereal::BinaryInputArchive& archive);

    // --- Member Variables ---
    asio::ip::tcp::socket socket; // Socket for this client connection
    ServerNetwork& serverNetwork; // Reference back to the main server network logic
    asio::strand<asio::io_context::executor_type>& gameStrand; // Strand for dispatching game logic calls

    // Buffer for reading incoming messages
    std::vector<char> readMsgBuffer;
    std::uint32_t incomingMsgSize; // Size of the message currently being read

    // Queue for outgoing messages
    MessageQueue writeMsgs;

    std::string playerName; // Name associated with this session after successful join/login
    bool joined = false; // Flag indicating if the player has successfully joined
};


#endif // SERVER_NETWORK_HPP