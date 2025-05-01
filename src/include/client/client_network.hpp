#ifndef CLIENT_NETWORK_HPP
#define CLIENT_NETWORK_HPP

#include <memory>
#include <string>
#include <vector>
#include <deque>
#include <functional>
#include "game_state.hpp"
#include "network.hpp"

/**
 * @brief Handles the network connection from the client to the server.
 *
 * Manages connecting, sending player actions, receiving game state updates
 * and server messages, and handling serialization/deserialization.
 */
class ClientNetwork : public std::enable_shared_from_this<ClientNetwork> {
public:
    // --- Callback Types ---
    // Called when a new game state is received from the server.
    using GameStateCallback = std::function<void(const ClientGameState&)>;
    // Called when the server reports an error related to a client action.
    using ErrorCallback = std::function<void(const ActionError&)>;
    // Called for simple notifications like successful login or disconnection.
    using VoidCallback = std::function<void()>;
    // Called specifically when login fails, providing a reason.
    using LoginFailureCallback = std::function<void(const std::string&)>;


    /**
     * @brief Constructor.
     * @param ioContext The ASIO I/O context to run network operations on.
     */
    ClientNetwork(asio::io_context& ioContext);

    /**
     * @brief Destructor. Ensures disconnection.
     */
    ~ClientNetwork();

    // --- Connection Management ---

    /**
     * @brief Asynchronously resolves the server address and attempts to connect.
     *        Initiates the login sequence upon successful connection.
     * @param host Server hostname or IP address.
     * @param port Server port number (as a string).
     * @param playerName The name the player wants to use.
     */
    void connect(const std::string& host, const std::string& port, const std::string& playerName);

    /**
     * @brief Disconnects from the server and cleans up resources.
     *        Invokes the disconnect callback if set.
     */
    void disconnect();

    /**
     * @brief Checks if the client is currently connected to the server.
     * @return True if connected, false otherwise.
     */
    bool isConnected() const;

    // --- Sending Actions ---
    // These methods serialize the corresponding action and send it to the server.
    // They should only be called after a successful connection and login.

    void sendDrawDeck();
    void sendTakeDiscardPile();
    void sendMeld(const std::vector<MeldRequest>& requests);
    void sendDiscard(const Card& card);
    void sendRevert();

    // --- Setting Callbacks ---
    // Register functions to be called when specific events occur.

    void setOnGameStateUpdate(GameStateCallback callback);
    void setOnActionError(ErrorCallback callback);
    void setOnLoginSuccess(VoidCallback callback);
    void setOnLoginFailure(LoginFailureCallback callback);
    void setOnDisconnect(VoidCallback callback); // Called on unexpected or explicit disconnect

private:
    // --- Internal ASIO Handlers ---
    // These handle the steps of the asynchronous connection and read/write operations.

    void doResolve(const std::string& host, const std::string& port, const std::string& playerName);
    void handleResolve(const asio::error_code& ec, const asio::ip::tcp::resolver::results_type& endpoints, const std::string& playerName);
    void doConnect(const asio::ip::tcp::resolver::results_type& endpoints, const std::string& playerName);
    void handleConnect(const asio::error_code& ec, const std::string& playerName);

    // Sends the initial login message after physical connection.
    void sendLogin(const std::string& playerName);

    // Handles reading the message size header.
    void doReadHeader();
    void handleReadHeader(const asio::error_code& error, std::size_t bytes_transferred);
    // Handles reading the message body based on the header size.
    void doReadBody();
    void handleReadBody(const asio::error_code& error, std::size_t bytes_transferred);

    // Handles writing messages from the queue.
    void doWrite();
    void handleWrite(const asio::error_code& error, std::size_t bytes_transferred);

    // --- Message Processing ---

    /**
     * @brief Deserializes the received message body and triggers appropriate callbacks.
     */
    void processMessage();

    // --- Sending Logic ---

    /**
     * @brief Internal helper to queue a message for sending.
     *        Handles serialization and adding the size header.
     * @tparam T The type of the data payload.
     * @param msgType The type of the client message.
     * @param data The data payload to serialize and send.
     */
    template <typename T>
    void queueMessage(ClientMessageType msgType, const T& data);

    /**
     * @brief Internal helper to queue a message with no payload for sending.
     * @param msgType The type of the client message.
     */
    void queueMessage(ClientMessageType msgType);


    // --- Callback Invocation ---
    // Safely invokes the registered callbacks.

    void invokeGameStateCallback(const ClientGameState& state);
    void invokeActionErrorCallback(const ActionError& actionError);
    void invokeLoginSuccessCallback();
    void invokeLoginFailureCallback(const std::string& errorMsg);
    void invokeDisconnectCallback(const std::string& reason = "Disconnected");


    // --- Member Variables ---
    asio::io_context& ioContext; // Reference to the application's I/O context
    asio::ip::tcp::socket socket; // Socket for the server connection
    asio::ip::tcp::resolver resolver; // Used for resolving server address
    bool connected; // Flag indicating connection status

    // Buffers and message queue
    std::vector<char> readMsgBuffer; // Buffer for incoming messages
    std::uint32_t incomingMsgSize; // Size of the message currently being read
    std::deque<std::vector<char>> writeMsgs; // Queue for outgoing messages (serialized with header)

    // Callbacks provided by the client application
    GameStateCallback onGameStateUpdateCallback;
    ErrorCallback onActionErrorCallback;
    VoidCallback onLoginSuccessCallback;
    LoginFailureCallback onLoginFailureCallback;
    VoidCallback onDisconnectCallback;

    // Store player name after successful login attempt initiation
    std::string clientPlayerName;
};


#endif // CLIENT_NETWORK_HPP
