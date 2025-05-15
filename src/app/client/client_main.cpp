#include <iostream>
#include <string>
#include <asio.hpp>
#include <sstream>
#include <algorithm>
#include "game_state.hpp"
#include "spdlog/spdlog.h"
#include "spdlog/sinks/stdout_color_sinks.h"
#include "client/client_network.hpp"
#include "client/canasta_console.hpp"
#include "client/game_view.hpp"
#include "client/client_controller.hpp"

// Constants
constexpr int SERVER_PORT = 12345;

// Function to configure spdlog for clean output
void configureLogger() {
    auto console = spdlog::stdout_color_mt("game_logger");
    spdlog::set_default_logger(console);
    spdlog::set_pattern("%v"); // Only display the message (no timestamp, level, etc.)
}


int main(int argc, char* argv[]) {
    if (argc < 2) {
        spdlog::error("Player index not specified!");
        return 1;
    }

    GameView gameView;
    CanastaConsole console;

    try {
        configureLogger(); // Configure the logger

        asio::io_context ioContext;

        // Connect to the server
        auto clientNetwork = std::make_shared<ClientNetwork>(ioContext);

        auto clientController = std::make_shared<ClientController>(clientNetwork, gameView);
        clientController->connect("127.0.0.1", std::to_string(SERVER_PORT));

        ioContext.run();
    } catch (const std::exception& e) {
        spdlog::error("Error: {}", e.what());
        return 1;
    }

    return 0;
}
