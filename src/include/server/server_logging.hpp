#ifndef SERVER_LOGGING_HPP
#define SERVER_LOGGING_HPP

#include <spdlog/spdlog.h>

/**
 * @brief Initializes the logger for the server.
 * @details Sets up console and file sinks, and creates a directory for logs if it doesn't exist.
 */
void initLogger();

#endif // SERVER_LOGGING_HPP