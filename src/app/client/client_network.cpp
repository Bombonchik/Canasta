#include "client/client_network.hpp"
#include "spdlog/spdlog.h"

#include <sstream>
#include <vector>
#include <cstring> // For std::memcpy


// --- ClientNetwork Implementation ---

ClientNetwork::ClientNetwork(asio::io_context& ioCtx)
    :   ioContext(ioCtx),
        socket(ioCtx),
        resolver(ioCtx),
        connected(false),
        incomingMsgSize(0)
{
    spdlog::debug("ClientNetwork created.");
}

ClientNetwork::~ClientNetwork() {
    spdlog::debug("ClientNetwork destroyed.");
    // Ensure disconnection happens cleanly if not already disconnected
    if (isConnected()) {
        disconnect();
    }
}

// --- Connection Management ---

void ClientNetwork::connect(const std::string& host, const std::string& port, const std::string& playerName) {
    if (isConnected()) {
        spdlog::warn("Already connected or connecting.");
        return;
    }
    //spdlog::debug("Attempting to connect to {}:{} as '{}'", host, port, playerName);
    // Start the asynchronous resolve operation.
    doResolve(host, port, playerName);
}

void ClientNetwork::disconnect() {
    if (!isConnected()) {
        spdlog::warn("disconnect() called but not connected.");
        return;
    }
    spdlog::info("Disconnecting from server...");
    connected = false; // Mark as disconnected immediately

    // Post the socket closure to the io_context to ensure it happens
    // within the ASIO event loop, preventing race conditions.
    asio::post(ioContext, [this, self = shared_from_this()]() {
        asio::error_code ec;
        // Shutdown both send and receive sides of the socket.
        socket.shutdown(asio::ip::tcp::socket::shutdown_both, ec);
        if (ec && ec != asio::error::not_connected) { // Ignore "not connected" error
            spdlog::error("Socket shutdown error: {}", ec.message());
        }
        // Close the socket.
        socket.close(ec);
        if (ec) {
            spdlog::error("Socket close error: {}", ec.message());
        }
        spdlog::info("Socket closed.");
        // Notify the application layer about the disconnection.
        invokeDisconnectCallback("Disconnected by client request");
    });
}


bool ClientNetwork::isConnected() const {
    // Check both the flag and the socket state for robustness
    return connected && socket.is_open();
}

// --- Sending Actions ---

void ClientNetwork::sendDrawDeck() {
    spdlog::debug("Queueing DrawDeck request.");
    queueMessage(ClientMessageType::DrawDeck);
}

void ClientNetwork::sendTakeDiscardPile() {
    spdlog::debug("Queueing TakeDiscardPile request.");
    queueMessage(ClientMessageType::TakeDiscardPile);
}

void ClientNetwork::sendMeld(std::vector<MeldRequest> requests) {
    spdlog::debug("Queueing Meld request.");
    queueMessage(ClientMessageType::Meld, requests);
}

void ClientNetwork::sendDiscard(Card card) {
    spdlog::debug("Queueing Discard request for card: {}", card.toString());
    queueMessage(ClientMessageType::Discard, card);
}

void ClientNetwork::sendRevert() {
    spdlog::debug("Queueing Revert request.");
    queueMessage(ClientMessageType::Revert);
}

// --- Setting Callbacks ---

void ClientNetwork::setOnGameStateUpdate(GameStateCallback callback) {
    onGameStateUpdateCallback = std::move(callback);
}

void ClientNetwork::setOnActionError(ErrorCallback callback) {
    onActionErrorCallback = std::move(callback);
}

void ClientNetwork::setOnLoginSuccess(VoidCallback callback) {
    onLoginSuccessCallback = std::move(callback);
}

void ClientNetwork::setOnLoginFailure(LoginFailureCallback callback) {
    onLoginFailureCallback = std::move(callback);
}

void ClientNetwork::setOnDisconnect(VoidCallback callback) {
    onDisconnectCallback = std::move(callback);
}

// --- Internal ASIO Handlers ---

void ClientNetwork::doResolve(const std::string& host, const std::string& port, const std::string& playerName) {
    // Start asynchronous resolve operation
    resolver.async_resolve(host, port,
        [this, self = shared_from_this(), playerName](const asio::error_code& ec, const asio::ip::tcp::resolver::results_type& endpoints) {
            handleResolve(ec, endpoints, playerName);
        });
}

void ClientNetwork::handleResolve(const asio::error_code& ec, const asio::ip::tcp::resolver::results_type& endpoints, const std::string& playerName) {
    if (!ec) {
        spdlog::debug("Resolve successful.");
        // Resolve successful, attempt to connect to the first endpoint.
        doConnect(endpoints, playerName);
    } else {
        spdlog::error("Resolve failed: {}", ec.message());
        // Handle resolve error (e.g., notify UI)
        invokeDisconnectCallback("Resolve failed: " + ec.message());
    }
}

void ClientNetwork::doConnect(const asio::ip::tcp::resolver::results_type& endpoints, const std::string& playerName) {
    // Start asynchronous connect operation
    asio::async_connect(socket, endpoints,
        [this, self = shared_from_this(), playerName](const asio::error_code& ec, const asio::ip::tcp::endpoint& /*endpoint*/) {
            handleConnect(ec, playerName);
        });
}

void ClientNetwork::handleConnect(const asio::error_code& ec, const std::string& playerName) {
    if (!ec) {
        //spdlog::debug("Connection successful");
        connected = true; // Mark as connected
        clientPlayerName = playerName; // Store player name

        // Connection successful, send the login message
        sendLogin(playerName);

        // Start reading messages from the server
        doReadHeader();
    } else {
        spdlog::error("Connect failed: {}", ec.message());
        connected = false; // Ensure disconnected state
        // Handle connection error (e.g., notify UI)
        invokeDisconnectCallback("Connect failed: " + ec.message());
        // Clean up socket if connection failed
        asio::error_code close_ec;
        socket.close(close_ec);
    }
}

void ClientNetwork::sendLogin(const std::string& playerName) {
    spdlog::debug("Sending Login message for player '{}'", playerName);
    queueMessage(ClientMessageType::Login, playerName);
}


void ClientNetwork::doReadHeader() {
    if (!isConnected()) return; // Don't read if not connected

    readMsgBuffer.resize(sizeof(std::uint32_t)); // Prepare buffer for 4-byte size header
    asio::async_read(socket, asio::buffer(readMsgBuffer),
        [this, self = shared_from_this()](const asio::error_code& error, std::size_t bytes_transferred) {
            handleReadHeader(error, bytes_transferred);
        });
}

void ClientNetwork::handleReadHeader(const asio::error_code& error, std::size_t /*bytes_transferred*/) {
    if (!isConnected()) return; // Check connection status again

    if (!error) {
        // Copy header data and convert from network to host byte order
        std::memcpy(&incomingMsgSize, readMsgBuffer.data(), sizeof(std::uint32_t));
        incomingMsgSize = asio::detail::socket_ops::network_to_host_long(incomingMsgSize);

        // Basic sanity check for message size
        if (incomingMsgSize > 65536) { // Example limit: 64k
            spdlog::error("Error: Incoming message size too large ({})", incomingMsgSize);
            disconnect(); // Disconnect on potentially malicious message
            return;
        }
        // Proceed to read the message body
        doReadBody();
    } else {
        if (error == asio::error::eof) {
            spdlog::info("Server closed the connection (EOF).");
            invokeDisconnectCallback("Server closed connection");
        } else if (error != asio::error::operation_aborted) {
            spdlog::error("Error reading header: {}", error.message());
            invokeDisconnectCallback("Read header error: " + error.message());
        } else {
            spdlog::debug("Read header operation aborted (likely during disconnect).");
        }
        // If an error occurred (EOF or other), ensure we are marked as disconnected
        if (connected) { // Avoid calling disconnect if already called
            disconnect();
        }
    }
}

void ClientNetwork::doReadBody() {
    if (!isConnected()) return;

    readMsgBuffer.resize(incomingMsgSize); // Resize buffer for the message body
    asio::async_read(socket, asio::buffer(readMsgBuffer),
        [this, self = shared_from_this()](const asio::error_code& error, std::size_t bytes_transferred) {
            handleReadBody(error, bytes_transferred);
        });
}

void ClientNetwork::handleReadBody(const asio::error_code& error, std::size_t /*bytes_transferred*/) {
    if (!isConnected()) return;

    if (!error) {
        // Message body read successfully, process it
        processMessage();
        // Start reading the next message header
        doReadHeader();
    } else {
        if (error == asio::error::eof) {
            spdlog::info("Server closed the connection (EOF) while reading body.");
            invokeDisconnectCallback("Server closed connection");
        } else if (error != asio::error::operation_aborted) {
            spdlog::error("Error reading body: {}", error.message());
            invokeDisconnectCallback("Read body error: " + error.message());
        } else {
            spdlog::debug("Read body operation aborted (likely during disconnect).");
        }
        // If an error occurred (EOF or other), ensure we are marked as disconnected
        if (connected) { // Avoid calling disconnect if already called
            disconnect();
        }
    }
}

void ClientNetwork::doWrite() {
    if (!isConnected() || writeMsgs.empty()) return;

    // Start asynchronous write of the first message in the queue
    asio::async_write(socket, asio::buffer(writeMsgs.front()),
        [this, self = shared_from_this()](const asio::error_code& error, std::size_t bytes_transferred) {
            handleWrite(error, bytes_transferred);
        });
}

void ClientNetwork::handleWrite(const asio::error_code& error, std::size_t /*bytes_transferred*/) {
    if (!isConnected()) return; // Check connection status

    if (!error) {
        // Write successful, remove the message from the queue
        writeMsgs.pop_front();
        // If there are more messages, start the next write
        if (!writeMsgs.empty()) {
            doWrite();
        }
    } else {
        if (error != asio::error::operation_aborted) {
            spdlog::error("Error writing message: {}", error.message());
            invokeDisconnectCallback("Write error: " + error.message());
            if (connected) { // Avoid calling disconnect if already called
                disconnect();
            }
        } else {
            spdlog::debug("Write operation aborted (likely during disconnect).");
        }
    }
}

// --- Message Processing ---

void ClientNetwork::processMessage() {
    ServerMessageType msgType;
    try {
        std::istringstream is(std::string(readMsgBuffer.begin(), readMsgBuffer.end()), std::ios::binary);
        cereal::BinaryInputArchive archive(is);

        archive(msgType); // Read the message type first

        spdlog::trace("Received message of type: {}", static_cast<int>(msgType));

        switch (msgType) {
            case ServerMessageType::GameStateUpdate: {
                ClientGameState gameState;
                archive(gameState); // Deserialize the game state
                invokeGameStateCallback(gameState);
                break;
            }
            case ServerMessageType::ActionError: {
                ActionError errorMsg;
                archive(errorMsg); // Deserialize the error message
                invokeActionErrorCallback(errorMsg);
                break;
            }
            case ServerMessageType::LoginSuccess: {
                spdlog::debug("Login successful for player '{}'", clientPlayerName);
                invokeLoginSuccessCallback();
                break;
            }
            case ServerMessageType::LoginFailure: {
                std::string reason;
                archive(reason); // Deserialize the failure reason
                spdlog::error("Login failed: {}", reason);
                invokeLoginFailureCallback(reason);
                // Login failed, so disconnect
                disconnect();
                break;
            }
            default:
                spdlog::error("Received unknown message type from server: {}", static_cast<int>(msgType));
                // Optionally disconnect on unknown message type
                // disconnect();
                break;
        }
    } catch (const cereal::Exception& e) {
        spdlog::error("Deserialization error (MsgType: {}): {}", static_cast<int>(msgType), e.what());
        // Disconnect on bad message format
        disconnect();
    } catch (const std::exception& e) {
        spdlog::error("Error processing message: {}", e.what());
        // Disconnect on other processing errors
        disconnect();
    }
}

// --- Sending Logic ---

template <typename T>
void ClientNetwork::queueMessage(ClientMessageType msgType, const T& data) {
    if (!isConnected()) {
        spdlog::warn("Cannot queue message: Not connected.");
        return;
    }
    // Serialize the message with header
    std::vector<char> message = serializeMessage(msgType, data);

    // Post the queuing and potential write start to the io_context strand
    // to ensure thread safety with the write queue.
    asio::post(ioContext, [this, self = shared_from_this(), message]() {
        bool writeInProgress = !writeMsgs.empty();
        writeMsgs.push_back(std::move(message));
        // If no write was in progress, start writing the new message
        if (!writeInProgress) {
            doWrite();
        }
    });
}

void ClientNetwork::queueMessage(ClientMessageType msgType) {
    if (!isConnected()) {
        spdlog::warn("Cannot queue message: Not connected.");
        return;
    }
    // Serialize the message (type only) with header
    std::vector<char> message = serializeMessage(msgType);

    asio::post(ioContext, [this, self = shared_from_this(), message]() {
        bool writeInProgress = !writeMsgs.empty();
        writeMsgs.push_back(std::move(message));
        if (!writeInProgress) {
            doWrite();
        }
    });
}


// --- Callback Invocation ---
// These helpers ensure callbacks are only called if they are set.

void ClientNetwork::invokeGameStateCallback(const ClientGameState& state) {
    if (onGameStateUpdateCallback) {
        try {
            onGameStateUpdateCallback(state);
        } catch (const std::exception& e) {
            spdlog::error("Exception in onGameStateUpdateCallback: {}", e.what());
        }
    } else {
        spdlog::warn("Received GameStateUpdate but no callback is set.");
    }
}

void ClientNetwork::invokeActionErrorCallback(const ActionError& actionError) {
    if (onActionErrorCallback) {
        try {
            onActionErrorCallback(actionError);
        } catch (const std::exception& e) {
            spdlog::error("Exception in onActionErrorCallback: {}", e.what());
        }
    } else {
        spdlog::warn("Received ActionError ('{}') but no callback is set.", actionError.message);
    }
}

void ClientNetwork::invokeLoginSuccessCallback() {
    if (onLoginSuccessCallback) {
        try {
            onLoginSuccessCallback();
        } catch (const std::exception& e) {
            spdlog::error("Exception in onLoginSuccessCallback: {}", e.what());
        }
    } else {
        spdlog::warn("Received LoginSuccess but no callback is set.");
    }
}

void ClientNetwork::invokeLoginFailureCallback(const std::string& errorMsg) {
    if (onLoginFailureCallback) {
        try {
            onLoginFailureCallback(errorMsg);
        } catch (const std::exception& e) {
            spdlog::error("Exception in onLoginFailureCallback: {}", e.what());
        }
    } else {
        spdlog::warn("Received LoginFailure ('{}') but no callback is set.", errorMsg);
    }
}

void ClientNetwork::invokeDisconnectCallback(const std::string& reason) {
    // Ensure disconnect callback is only called once per disconnection event
    static bool disconnectCallbackInvoked = false;
    if (onDisconnectCallback && !disconnectCallbackInvoked) {
        disconnectCallbackInvoked = true; // Set flag immediately
        spdlog::info("Invoking disconnect callback. Reason: {}", reason);
        try {
            onDisconnectCallback();
        } catch (const std::exception& e) {
            spdlog::error("Exception in onDisconnectCallback: {}", e.what());
        }
        // Reset flag after invocation (or maybe not, depending on desired behavior)
        // If strictly once per ClientNetwork lifetime, don't reset.
        // If once per connection attempt/disconnection event, reset might be needed
        // depending on how connect/disconnect logic is structured. Let's keep it simple for now.
        // disconnectCallbackInvoked = false; // Reset if needed
    } else if (!onDisconnectCallback) {
        spdlog::warn("Disconnected ('{}') but no disconnect callback is set.", reason);
    } else {
        spdlog::debug("Disconnect callback already invoked for this event.");
    }
}