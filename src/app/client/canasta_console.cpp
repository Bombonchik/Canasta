#include "client/canasta_console.hpp"

#include <iostream>

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

void CanastaConsole::printNewLine() {
    print("\n");
}

void CanastaConsole::printSpace(std::size_t count) {
    for (std::size_t i = 0; i < count; ++i) {
        print(" ");
    }
}

void CanastaConsole::clear() {
    // ANSI: clear screen and move cursor home
    // std::cout << "\x1b[2J\x1b[H";
    // std::cout.flush();
#ifdef _WIN32
    system("cls");
#else
    system("clear");
#endif
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
