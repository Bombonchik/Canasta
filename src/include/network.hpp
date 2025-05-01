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

// Should match the enum in server_network.hpp
enum class ClientMessageType : uint8_t {
    Login,
    DrawDeck,
    TakeDiscardPile,
    Meld,
    Discard,
    Revert,
};

// Message types received BY Client FROM Server
// Should match the enum in server_network.cpp
enum class ServerMessageType : uint8_t {
    GameStateUpdate,
    ActionError,
    LoginSuccess,
    LoginFailure,
};

struct ActionError {
    std::string     message;
    std::optional<TurnActionStatus> status;

    template <class Archive>
    void serialize(Archive& ar) {
        ar(CEREAL_NVP(message), CEREAL_NVP(status));
    }
};

// Helper function to serialize data with size header

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

// Overload for messages with only a type (no data payload)
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