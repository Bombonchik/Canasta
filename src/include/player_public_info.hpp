#ifndef PLAYER_PUBLIC_INFO_HPP
#define PLAYER_PUBLIC_INFO_HPP

#include <string>
#include "cereal/cereal.hpp"
#include "cereal/archives/binary.hpp"

/**
 * @struct PlayerPublicInfo
 * @brief Struct representing public information about a player.
 * @details This struct is used to send player information to clients.
 */
struct PlayerPublicInfo {
    std::string name;
    std::size_t handCardCount;
    bool isCurrentTurn; // Flag if this player is the current one

    template <class Archive>
    void serialize(Archive& ar) {
        ar(CEREAL_NVP(name), CEREAL_NVP(handCardCount), CEREAL_NVP(isCurrentTurn));
    }
};

#endif // PLAYER_PUBLIC_INFO_HPP