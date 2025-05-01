#include "server/server_network.hpp"
#include "server/game_manager.hpp"
#include "server/turn_manager.hpp" // TurnActionResult, MeldRequest
#include "game_state.hpp"   // ClientGameState
#include "player.hpp"       // Needed for assembling state
#include "server/make_state.hpp"
#include "spdlog/spdlog.h"
#include <stdexcept>
#include <vector>


// --- Session Implementation ---

Session::Session(asio::ip::tcp::socket socket, ServerNetwork& serverNetwork, asio::strand<asio::io_context::executor_type>& gameStrand)
    :   socket(std::move(socket)),
        serverNetwork(serverNetwork),
        gameStrand(gameStrand),
        incomingMsgSize(0),
        joined(false)
{}

void Session::start() {
    // Initially, expect a Login message from the client
    spdlog::info("Session started for {}. Waiting for Login.", socket.remote_endpoint().address().to_string());
    doReadHeader();
}

void Session::deliver(const std::vector<char>& message) {
    // Post the write operation to the socket's implicit strand
    asio::post(socket.get_executor(),
        [this, self = shared_from_this(), message]() {
            bool writeInProgress = !writeMsgs.empty();
            writeMsgs.push_back(message);
            if (!writeInProgress) {
                doWrite();
            }
        });
}

const std::string& Session::getPlayerName() const {
    return playerName;
}

void Session::doReadHeader() {
    readMsgBuffer.resize(sizeof(std::uint32_t)); // Prepare buffer for 4-byte size header
    asio::async_read(socket, asio::buffer(readMsgBuffer),
        [this, self = shared_from_this()](const asio::error_code& error, std::size_t bytes_transferred) {
            handleReadHeader(error, bytes_transferred);
        });
}

void Session::handleReadHeader(const asio::error_code& error, std::size_t /*bytes_transferred*/) {
    if (!error) {
        // Copy header data and convert from network to host byte order
        std::memcpy(&incomingMsgSize, readMsgBuffer.data(), sizeof(std::uint32_t));
        incomingMsgSize = asio::detail::socket_ops::network_to_host_long(incomingMsgSize);

        // Basic sanity check for message size (e.g., prevent huge allocations)
        if (incomingMsgSize > 65536) { // Example limit: 64k
            spdlog::error("Error: Incoming message size too large ({}) from {}", incomingMsgSize, playerName);
            serverNetwork.leave(shared_from_this()); // Disconnect client
            return;
        }

        doReadBody();
    } else {
        if (error != asio::error::operation_aborted) {
            spdlog::error("Error reading header from {}: {}", playerName, error.message());
            serverNetwork.leave(shared_from_this());
        }
         // else: operation aborted, likely during shutdown, do nothing
    }
}

void Session::doReadBody() {
    readMsgBuffer.resize(incomingMsgSize); // Resize buffer for the message body
    asio::async_read(socket, asio::buffer(readMsgBuffer),
        [this, self = shared_from_this()](const asio::error_code& error, std::size_t bytes_transferred) {
            handleReadBody(error, bytes_transferred);
        });
}

void Session::handleReadBody(const asio::error_code& error, std::size_t /*bytes_transferred*/) {
    if (!error) {
        processMessage(); // Process the complete message
        doReadHeader();   // Start reading the next message header
    } else {
        if (error != asio::error::operation_aborted) {
            spdlog::error("Error reading body from {}: {}", playerName, error.message());
            serverNetwork.leave(shared_from_this());
        }
          // else: operation aborted
    }
}

void Session::doWrite() {
    if (writeMsgs.empty()) return; // Should not happen if called correctly

    asio::async_write(socket, asio::buffer(writeMsgs.front()),
        [this, self = shared_from_this()](const asio::error_code& error, std::size_t bytes_transferred) {
            handleWrite(error, bytes_transferred);
        });
}

void Session::handleWrite(const asio::error_code& error, std::size_t /*bytes_transferred*/) {
    if (!error) {
        writeMsgs.pop_front();
        if (!writeMsgs.empty()) {
            doWrite(); // Write next message in queue
        }
    } else {
        if (error != asio::error::operation_aborted) {
            spdlog::error("Error writing to {}: {}", playerName, error.message());
            serverNetwork.leave(shared_from_this());
        } else {
            spdlog::debug("Write operation aborted (likely during disconnect).");
        }
    }
}

void Session::processMessage() {
    ClientMessageType msgType;
    try {
        std::istringstream is(std::string(readMsgBuffer.begin(), readMsgBuffer.end()), std::ios::binary);
        cereal::BinaryInputArchive archive(is);

        archive(msgType); // Read the message type first

        if (!joined) {
            // Only accept Login message if not joined
            processLoginMessage(msgType, archive);
        } else {
            // If joined, process game actions. Deserialize payload *before* posting to strand.
            processGameMessage(msgType, archive);
        }
    } catch (const cereal::Exception& e) {
        spdlog::error("Deserialization error for {} (MsgType: {}): {}", playerName, static_cast<int>(msgType), e.what());
        // Consider disconnecting client on bad message format
        serverNetwork.leave(shared_from_this());
    } catch (const std::exception& e) {
        spdlog::error("Error processing message for {}: {}", playerName, e.what());
        // Consider disconnecting client
        serverNetwork.leave(shared_from_this());
    }
}

void Session::processLoginMessage(ClientMessageType msgType, cereal::BinaryInputArchive& archive) {
    if (msgType != ClientMessageType::Login) {
        spdlog::error("Expected Login but got {} from unjoined client {}. Ignoring.", 
            int(msgType), socket.remote_endpoint().address().to_string());
        return; // or call serverNetwork.leave(...)
    }
    std::string nameAttempt;
    archive(nameAttempt); // Deserialize player name
    // Dispatch login attempt to the server network logic (which might check name validity/availability)
    asio::post(serverNetwork.ioContext, [this, self = shared_from_this(), nameAttempt]() {
        // Basic validation: Check if name is empty
        if (nameAttempt.empty()) {
            spdlog::error("Login failed: Empty name received.");
            auto errorMsg = serializeMessage(ServerMessageType::LoginFailure, std::string("Name cannot be empty."));
            deliver(errorMsg);
            return;
        }
        playerName = nameAttempt;
        auto joinStatus = serverNetwork.join(shared_from_this(), playerName);
        if (joinStatus.has_value()){
            joined = true;
        }
    });
}

void Session::processGameMessage(ClientMessageType msgType, cereal::BinaryInputArchive& archive) {
    switch (msgType) {
        case ClientMessageType::DrawDeck:
            asio::post(gameStrand, [this, self = shared_from_this()]() {
                serverNetwork.handleClientDrawDeck(playerName);
            });
            break;
        case ClientMessageType::TakeDiscardPile:
            asio::post(gameStrand, [this, self = shared_from_this()]() {
                serverNetwork.handleClientTakeDiscardPile(playerName);
            });
            break;
        case ClientMessageType::Meld:
            {
                std::vector<MeldRequest> requests;
                archive(requests); // Deserialize payload now
                asio::post(gameStrand, [this, self = shared_from_this(), requests /* capture data */]() {
                    serverNetwork.handleClientMeld(playerName, requests);
                });
            }
            break;
        case ClientMessageType::Discard:
            {
                Card cardToDiscard;
                archive(cardToDiscard); // Deserialize payload now
                asio::post(gameStrand, [this, self = shared_from_this(), cardToDiscard /* capture data */]() {
                    serverNetwork.handleClientDiscard(playerName, cardToDiscard);
                });
            }
            break;
        case ClientMessageType::Revert:
            asio::post(gameStrand, [this, self = shared_from_this()]() {
                serverNetwork.handleClientRevert(playerName);
            });
            break;
        case ClientMessageType::Login:
            // Ignore login message if already joined
            spdlog::warn("Warning: Received Login message from already joined player '{}'", playerName);
            break;
        default:
            spdlog::error("Received unknown message type from {}", playerName);
            // Optionally send an error message back or disconnect
            break;
    }
}


// --- ServerNetwork Implementation ---

ServerNetwork::ServerNetwork(asio::io_context& ioContext,
                                const asio::ip::tcp::endpoint& endpoint,
                                GameManager& gameManager)
    :   ioContext(ioContext),
        acceptor(ioContext, endpoint),
        gameManager(gameManager),
        gameStrand(asio::make_strand(ioContext)) // Initialize the strand
{
    spdlog::info("ServerNetwork created. Listening on {}", endpoint.address().to_string());
}

void ServerNetwork::startAccept() {
    acceptor.async_accept(
        [this](const asio::error_code& error, asio::ip::tcp::socket socket) {
            if (!error) {
                // Create session on the io_context, pass gameStrand
                auto newSession = std::make_shared<Session>(std::move(socket), *this, gameStrand);
                // Don't add to sessions map yet, wait for login message
                newSession->start(); // Start reading from the new client
            } else {
                spdlog::error("Accept error: {}", error.message());
            }
            // Continue accepting connections regardless of error on this one
            startAccept();
        });
}

Status ServerNetwork::join(SessionPtr session, const std::string& playerName) {
    // This should ideally run on the main io_context thread or be protected
    {
        std::lock_guard<std::mutex> lock(sessionsMutex);
        if (sessions.count(playerName) > 0) {
            spdlog::error("Player name '{}' already taken.", playerName);
            auto errorMsg = serializeMessage(ServerMessageType::LoginFailure, std::string("Name already taken."));
            session->deliver(errorMsg);
            return std::unexpected("Name already taken.");
        }
        if (gameManager.allPlayersJoined()) {
            spdlog::error("Game is full. Cannot join.");
            auto errorMsg = serializeMessage(ServerMessageType::LoginFailure, std::string("Game is full."));
            session->deliver(errorMsg);
            return std::unexpected("Game is full.");
        }
        sessions[playerName] = session;
        // Send LoginSuccess confirmation
        auto successMsg = serializeMessage(ServerMessageType::LoginSuccess);
        session->deliver(successMsg);
        spdlog::info("Player '{}' joined.", playerName);
    }
    asio::post(gameStrand, [this, playerName = std::move(playerName)]() mutable {
        auto status = gameManager.addPlayer(playerName);
        if (!status.has_value()) {
            // this should never happen—names/counts already checked above—but if it does we log an error.
            spdlog::error("addPlayer failed on strand: {}", status.error());
        }

        // once everyone has joined, we can start the game:
        if (gameManager.allPlayersJoined()) {
            gameManager.startGame();
            broadcastGameState("Game started!");
        }
    });
    return {}; // Indicate success
}

void ServerNetwork::leave(SessionPtr session) {
    // This can be called from session's strand or acceptor's thread
    std::lock_guard<std::mutex> lock(sessionsMutex);
    std::string nameToRemove = session->getPlayerName();
    if (!nameToRemove.empty()) {
        sessions.erase(nameToRemove);
        spdlog::info("Player '{}' left → shutting down server..", nameToRemove);
        
        asio::error_code ec;
        acceptor.close(ec);
        if (ec) spdlog::warn("Error closing acceptor: {}", ec.message());
        // 2) abort all outstanding async ops and break run():
        ioContext.stop();
        gameManager.handlePlayerDisconnect(nameToRemove); // Notify game manager
    } else {
        spdlog::info("Unidentified session disconnected.");
    }
    // Session object will be destroyed when shared_ptr count goes to 0
}

void ServerNetwork::deliverToOne(const std::string& playerName, const std::vector<char>& message) {
    std::lock_guard<std::mutex> lock(sessionsMutex);
    auto it = sessions.find(playerName);
    if (it != sessions.end()) {
        it->second->deliver(message);
    } else {
        spdlog::warn("Warning: Attempted to deliver message to unknown player: {}", playerName);
    }
}

void ServerNetwork::deliverToAll(const std::vector<char>& message) {
    std::lock_guard<std::mutex> lock(sessionsMutex);
    for (const auto& pair : sessions) {
        pair.second->deliver(message);
    }
}

// --- Action Handlers (Called via gameStrand from Session::processMessage) ---

// Helper to send state updates after successful action
void ServerNetwork::broadcastGameState(const std::string& lastActionMsg, std::optional<TurnActionStatus> status) {
     // This function MUST run on the gameStrand
    assert(gameStrand.running_in_this_thread());

    bool broadcastNewRound = false;
    if (auto* roundManager = gameManager.getCurrentRoundManager()) {
        if (roundManager->isRoundOver()) {
            spdlog::info("Round is over. Advancing game state.");
            broadcastNewRound = true;
            gameManager.advanceGameState(); // Advance game state
        }
    }

    if (const auto* roundManager = gameManager.getCurrentRoundManager()) {
        for (const auto& player : gameManager.getAllPlayers()) {
            const std::string& targetPlayerName = player.getName();
    
            // Check if this player is actually connected
            SessionPtr targetSession;
            { // Scope for lock
                std::lock_guard<std::mutex> lock(sessionsMutex);
                auto it = sessions.find(targetPlayerName);
                if (it != sessions.end()) {
                    targetSession = it->second;
                }
            } // Unlock mutex
    
            if (!targetSession) {
                spdlog::warn("Warning: Attempted to send game state to disconnected player: {}", targetPlayerName);
                continue; // Skip if player is not connected
            }
            // Only send if player is connected
            try {
                ClientGameState clientGameState = makeClientGameState(
                    player, *roundManager, gameManager, lastActionMsg, status
                );
                auto message = serializeMessage(ServerMessageType::GameStateUpdate, clientGameState);
                targetSession->deliver(message); // Deliver uses session's post, safe
    
            } catch (const std::exception& e) {
                spdlog::error("Error assembling or serializing game state for {}: {}", targetPlayerName, e.what());
            }
        }
    }

    if (broadcastNewRound) {
        gameManager.advanceGameState();
        broadcastGameState("New round started!");
    }
}

// Helper to send error message back to originating player
void ServerNetwork::sendActionError(const std::string& playerName, const std::string& errorMsg, std::optional<TurnActionStatus> status) {
    // This function MUST run on the gameStrand
    assert(gameStrand.running_in_this_thread());
    spdlog::error("Action Error for {}: {}", playerName, errorMsg);
    ActionError actionError {
        errorMsg,
        status
    };
    auto message = serializeMessage(ServerMessageType::ActionError, actionError);
    deliverToOne(playerName, message); // deliverToOne is thread-safe
}

void ServerNetwork::handleClientDrawDeck(const std::string& playerName) {
    dispatchAction(playerName, [&](RoundManager& rm) {
        return rm.handleDrawDeckRequest();
    });
}

void ServerNetwork::handleClientTakeDiscardPile(const std::string& playerName) {
    dispatchAction(playerName, [&](RoundManager& rm) {
        return rm.handleTakeDiscardPileRequest();
    });
}

void ServerNetwork::handleClientMeld(const std::string& playerName, const std::vector<MeldRequest>& meldRequests) {
    dispatchAction(playerName, [&](RoundManager& rm) {
        return rm.handleMeldRequest(meldRequests);
    });
}

void ServerNetwork::handleClientDiscard(const std::string& playerName, const Card& cardToDiscard) {
    dispatchAction(playerName, [&](RoundManager& rm) {
        return rm.handleDiscardRequest(cardToDiscard);
    });
}

void ServerNetwork::handleClientRevert(const std::string& playerName) {
    dispatchAction(playerName, [&](RoundManager& rm) {
        return rm.handleRevertRequest();
    });
}