#ifndef PLAYER_PUBLIC_INFO_HPP
#define PLAYER_PUBLIC_INFO_HPP

#include <string>
#include "cereal/cereal.hpp"
#include "cereal/archives/binary.hpp"

/**
 * @class PlayerPublicInfo
 * @brief Class representing public information about a player.
 * @details This class is used to send player information to clients.
 */
class PlayerPublicInfo {
private:
    std::string name;
    std::size_t handCardCount;
    bool isCurrentTurn; // Flag if this player is the current one
public:
    /**
     * @brief Default constructor for PlayerPublicInfo for serialization purposes.
     */
    PlayerPublicInfo() = default;
    /**
     * @brief Default constructor for PlayerPublicInfo.
     */
    PlayerPublicInfo(const std::string& playerName, std::size_t cardCount, bool currentTurn)
        : name(playerName), handCardCount(cardCount), isCurrentTurn(currentTurn) {}

    const std::string& getName() const { return name; }
    std::size_t getHandCardCount() const { return handCardCount; }
    bool isCurrentPlayer() const { return isCurrentTurn; }

    template <class Archive>
    void serialize(Archive& ar) {
        ar(CEREAL_NVP(name), CEREAL_NVP(handCardCount), CEREAL_NVP(isCurrentTurn));
    }
};

#endif // PLAYER_PUBLIC_INFO_HPP