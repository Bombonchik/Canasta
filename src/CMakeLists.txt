cmake_minimum_required(VERSION 3.28)
project(MyCanastaGame)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find required packages
find_package(SQLiteCpp REQUIRED)
find_package(spdlog REQUIRED)
find_package(asio REQUIRED)
find_package(cereal REQUIRED)

# --- Core Game Logic Library ---
add_library(canasta_core STATIC
    app/card.cpp
    app/meld.cpp
    # Add future .cpp files here:
    app/hand.cpp
    app/player.cpp
    # app/round.cpp
    app/team.cpp
    app/rule_engine.cpp
    app/team_round_state.cpp
)

# Specify include directory for the core library
target_include_directories(canasta_core PUBLIC include)

# Link core library against dependencies needed for its headers/compilation
target_link_libraries(canasta_core PUBLIC
    cereal::cereal
)


# --- Server Executable ---
add_executable(canasta_server
    app/server/server_main.cpp
    app/server/server_deck.cpp
    app/server/turn_manager.cpp # Added TurnManager source
    app/server/server_logging.cpp # Added server logging source
    # Removed app/client/client_deck.cpp from here
)

# Specify include directory for the server
target_include_directories(canasta_server PRIVATE include)

# Link server against core library and dependencies
target_link_libraries(canasta_server PRIVATE
    canasta_core
    SQLiteCpp       # Only server needs DB?
    spdlog::spdlog
    asio::asio
    # cereal::cereal is inherited via canasta_core PUBLIC link
)

# --- Client Executable ---
add_executable(canasta_client
    app/client/client_main.cpp
    app/client/client_deck.cpp
)

# Specify include directory for the client
target_include_directories(canasta_client PRIVATE include)

# Link client against core library and dependencies
target_link_libraries(canasta_client PRIVATE
    canasta_core
    spdlog::spdlog
    asio::asio
    # cereal::cereal is inherited via canasta_core PUBLIC link
)
