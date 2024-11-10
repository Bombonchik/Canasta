#ifndef GAME_STATE_HPP
#define GAME_STATE_HPP

#include <string>
#include "cereal/cereal.hpp"
#include "cereal/archives/binary.hpp"

struct GameState {
    int currentPlayer;
    std::string publicMessage;

    template <class Archive>
    void serialize(Archive& archive) {
        archive(currentPlayer, publicMessage);
    }
};

#endif //GAME_STATE_HPP
