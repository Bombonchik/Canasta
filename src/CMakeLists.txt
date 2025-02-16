cmake_minimum_required(VERSION 3.28)
project(MyCanastaGame)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add include directories for header files
include_directories(include)

# Use a relative path to the Conan toolchain file from the build directory
find_package(SQLiteCpp REQUIRED)
find_package(spdlog REQUIRED)
find_package(asio REQUIRED)
find_package(cereal REQUIRED)

# Define your executables
add_executable(canasta_server
        app/server/server_main.cpp)
add_executable(canasta_client
        app/client/client_main.cpp
        include/game_state.hpp)

# Link libraries provided by Conan
#target_link_libraries(launcher sqlitecpp sqlite3)
target_link_libraries(canasta_server SQLiteCpp spdlog::spdlog asio::asio cereal::cereal)
target_link_libraries(canasta_client SQLiteCpp spdlog::spdlog asio::asio cereal::cereal)
