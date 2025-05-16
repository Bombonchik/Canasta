#ifndef NETWORK_HPP
#define NETWORK_HPP

#include <asio.hpp>
#include <asio/error.hpp> // For error codes
#include <asio/post.hpp> // For asio::post
#include <cstdint> 
#include <vector>
#include <cereal/archives/binary.hpp>
#include <cereal/types/string.hpp>
#include <cereal/types/vector.hpp>
#include <cereal/types/optional.hpp>
#include <sstream>
#include <memory>
#include <cstring> // For std::memcpy
#include <optional>
#include "card.hpp"
#include "meld.hpp"
#include "rule_engine.hpp"

/**
 * @enum ClientMessageType
 * @brief Enum representing the type of the action sent from the client to the server.
 */
enum class ClientMessageType : uint8_t {
    Login,
    DrawDeck,
    TakeDiscardPile,
    Meld,
    Discard,
    Revert,
};

/**
 * @enum ServerMessageType
 * @brief Enum representing the type of a message sent from the server to the client.
 */
enum class ServerMessageType : uint8_t {
    GameStateUpdate,
    ActionError,
    LoginSuccess,
    LoginFailure,
};

/**
 * @class ActionError
 * @brief Class representing an error message sent from the server to the client.
 * @details Contains a message and an optional status code.
 */
class ActionError {
private:
    std::string     message;
    std::optional<TurnActionStatus> status;
public:
    ActionError() = default; // Default constructor for serialization
    ActionError(const std::string& msg, std::optional<TurnActionStatus> stat = std::nullopt)
        : message(msg), status(stat) {}

    const std::string& getMessage() const { return message; }
    std::optional<TurnActionStatus> getStatus() const { return status; }

    template <class Archive>
    void serialize(Archive& ar) {
        ar(CEREAL_NVP(message), CEREAL_NVP(status));
    }
};

// Helper function to serialize data with size header

/**
 * @brief Serialize a message of type M with additional data.
 * @tparam M Message type (ClientMessageType or ServerMessageType)
 * @tparam T Data type to serialize
 * @param msgType Message type
 * @param data Data to serialize
 * @return Serialized message as a vector of chars
 */
template <typename M, typename T> // M = ClientMessageType or ServerMessageType, T = some data type
std::vector<char> serializeMessage(M msgType, const T& data) {
    std::ostringstream os(std::ios::binary);
    { // Scope for the archive
        cereal::BinaryOutputArchive archive(os);
        archive(msgType); // Write message type first
        archive(data);    // Write the actual data payload
    } // Archive goes out of scope, flushes data to os

    std::string serializedData = os.str();
    std::uint32_t dataSize = static_cast<std::uint32_t>(serializedData.size());
    std::uint32_t networkDataSize = asio::detail::socket_ops::host_to_network_long(dataSize); // Ensure network byte order

    std::vector<char> messageBuffer;
    messageBuffer.resize(sizeof(networkDataSize) + dataSize);

    // Copy size header
    std::memcpy(messageBuffer.data(), &networkDataSize, sizeof(networkDataSize));
    // Copy serialized data
    std::memcpy(messageBuffer.data() + sizeof(networkDataSize), serializedData.data(), dataSize);

    return messageBuffer;
}

/**
 * @brief Serialize a message of type M without additional data.
 * @tparam M Message type (ClientMessageType or ServerMessageType)
 * @param msgType Message type
 * @return Serialized message as a vector of chars
 */
template <typename M>
std::vector<char> serializeMessage(M msgType) {
    std::ostringstream os(std::ios::binary);
    {
        cereal::BinaryOutputArchive archive(os);
        archive(msgType);
    }
    std::string serializedData = os.str();
    std::uint32_t dataSize = static_cast<std::uint32_t>(serializedData.size());
    std::uint32_t networkDataSize = asio::detail::socket_ops::host_to_network_long(dataSize);

    std::vector<char> messageBuffer;
    messageBuffer.resize(sizeof(networkDataSize) + dataSize);
    std::memcpy(messageBuffer.data(), &networkDataSize, sizeof(networkDataSize));
    std::memcpy(messageBuffer.data() + sizeof(networkDataSize), serializedData.data(), dataSize);
    return messageBuffer;
}

#endif // NETWORK_HPP