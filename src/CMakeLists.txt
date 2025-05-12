cmake_minimum_required(VERSION 3.28)
project(MyCanastaGame)

list(APPEND CMAKE_PREFIX_PATH "${CMAKE_BINARY_DIR}/generators")
list(APPEND CMAKE_MODULE_PATH "${CMAKE_BINARY_DIR}/generators")

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find required packages
find_package(spdlog REQUIRED)
find_package(asio REQUIRED)
find_package(cereal REQUIRED)
find_package(FTXUI REQUIRED
    COMPONENTS
    screen
    dom
    component
)

# --- Core Game Logic Library ---
add_library(canasta_core STATIC
    app/card.cpp
    app/hand.cpp
    app/meld.cpp
    app/player.cpp
    app/team.cpp
    app/rule_engine.cpp
    app/team_round_state.cpp
    app/client_deck.cpp
)

# Specify include directory for the core library
target_include_directories(canasta_core PUBLIC include)

# Link core library against dependencies needed for its headers/compilation
target_link_libraries(canasta_core PUBLIC
    cereal::cereal
    spdlog::spdlog
)


# --- Server Executable ---
add_executable(canasta_server
    app/server/server_logging.cpp
    app/server/server_main.cpp
    app/server/server_deck.cpp
    app/server/turn_manager.cpp
    app/server/round_manager.cpp
    app/server/game_manager.cpp
    app/server/server_network.cpp
)

# Specify include directory for the server
target_include_directories(canasta_server PRIVATE include)

# Link server against core library and dependencies
target_link_libraries(canasta_server PRIVATE
    canasta_core
    spdlog::spdlog
    asio::asio
    # cereal::cereal is inherited via canasta_core PUBLIC link
)


# --- Client Executable ---
add_executable(canasta_client
    app/client/client_main.cpp
    app/client/client_network.cpp
    app/client/canasta_console.cpp
    app/client/input_guard.cpp
    app/client/game_view.cpp
    app/client/client_controller.cpp
)

# Specify include directory for the client
target_include_directories(canasta_client PRIVATE include)

# Link client against core library and dependencies
target_link_libraries(canasta_client PRIVATE
    canasta_core
    spdlog::spdlog
    asio::asio
    ftxui::screen
    ftxui::dom
    ftxui::component
    # cereal::cereal is inherited via canasta_core PUBLIC link
)
