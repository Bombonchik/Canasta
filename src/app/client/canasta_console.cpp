#include "client/canasta_console.hpp"

/*CanastaConsole::CanastaConsole() {
    #ifdef _WIN32
        // Enable UTF-8 output and ANSI escape processing on Windows 10+
        SetConsoleOutputCP(CP_UTF8);
        HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
        DWORD mode = 0;
        GetConsoleMode(hOut, &mode);
        SetConsoleMode(hOut, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
    #endif
        // Create a normal color sink (with newline)
        auto sinkWithNL = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
        sinkWithNL->set_pattern("%v");  // only the message

        // Create a sink that does NOT append a newline
        auto sinkNoNL = std::make_shared<SinkNoNewline<spdlog::sinks::stdout_color_sink_mt>>();
        sinkNoNL->set_pattern("%v");

        // Build loggers (not registered globally)
        loggerWithNL = std::make_shared<spdlog::logger>("console_ui_nl", sinkWithNL);
        loggerNoNL = std::make_shared<spdlog::logger>("console_ui_nonl", sinkNoNL);

        // Always flush after every message
        loggerWithNL->flush_on(spdlog::level::info);
        loggerNoNL->flush_on(spdlog::level::info);
}

void CanastaConsole::print(const std::string& message,
    Color color,
    bool newline)
{
    const std::string& msg = applyColor(message, color);
    if (newline) {
        loggerWithNL->info(msg);
    } else {
        loggerNoNL->info(msg);
    }
}

void CanastaConsole::clear() {
    #ifdef _WIN32
        system("cls");
    #else
        system("clear");
    #endif
    // Clear the console and reset the cursor position
    if (loggerWithNL) {
        loggerWithNL->info("\033[H\033[J"); // ANSI escape codes to clear the screen
    }
}

std::string CanastaConsole::applyColor(const std::string& msg, Color c) {
    const char* code = nullptr;
    switch (c) {
        case Color::Red:         code = "\x1b[31m"; break;
        case Color::Green:       code = "\x1b[32m"; break;
        case Color::Yellow:      code = "\x1b[33m"; break;
        case Color::Blue:        code = "\x1b[34m"; break;
        case Color::Magenta:     code = "\x1b[35m"; break;
        case Color::Cyan:        code = "\x1b[36m"; break;
        case Color::White:       code = "\x1b[37m"; break;
        case Color::BrightRed:   code = "\x1b[91m"; break;
        case Color::BrightGreen: code = "\x1b[92m"; break;
        case Color::BrightYellow:code = "\x1b[93m"; break;
        case Color::BrightBlue:  code = "\x1b[94m"; break;
        case Color::BrightMagenta:code ="\x1b[95m"; break;
        case Color::BrightCyan:  code = "\x1b[96m"; break;
        case Color::BrightWhite: code = "\x1b[97m"; break;
        default: return msg;
    }
    return std::string(code) + msg + "\x1b[0m";
}*/

#include <iostream>

#ifdef _WIN32
#include <windows.h>
#endif

CanastaConsole::CanastaConsole() {
#ifdef _WIN32
    // Enable UTF-8 output
    SetConsoleOutputCP(CP_UTF8);
    // Enable ANSI escape codes
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hOut != INVALID_HANDLE_VALUE) {
        DWORD mode;
        if (GetConsoleMode(hOut, &mode)) {
            SetConsoleMode(hOut, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
        }
    }
#endif
}

void CanastaConsole::print(const std::string& msg,
                            Color color,
                            bool newline) {
    std::string out = applyColor(msg, color);
    if (newline) {
        std::cout << out << std::endl;
    } else {
        std::cout << out;
        std::cout.flush();
    }
}

void CanastaConsole::clear() {
    // ANSI: clear screen and move cursor home
    std::cout << "\x1b[2J\x1b[H";
    std::cout.flush();
}

std::string CanastaConsole::applyColor(const std::string& text, Color color) {
    // ANSI codes for each color enum (index matches Color)
    static constexpr const char* codes[] = {
        nullptr,        // Default
        "\x1b[31m",   // Red
        "\x1b[32m",   // Green
        "\x1b[33m",   // Yellow
        "\x1b[34m",   // Blue
        "\x1b[35m",   // Magenta
        "\x1b[36m",   // Cyan
        "\x1b[37m",   // White
        "\x1b[91m",   // BrightRed
        "\x1b[92m",   // BrightGreen
        "\x1b[93m",   // BrightYellow
        "\x1b[94m",   // BrightBlue
        "\x1b[95m",   // BrightMagenta
        "\x1b[96m",   // BrightCyan
        "\x1b[97m"    // BrightWhite
    };
    int idx = static_cast<int>(color);
    if (idx <= 0 || idx >= static_cast<int>(sizeof(codes)/sizeof(codes[0])) || !codes[idx]) {
        return text;
    }
    return std::string(codes[idx]) + text + "\x1b[0m";
}
