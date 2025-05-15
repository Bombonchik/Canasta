// input_guard.cpp
#include "client/input_guard.hpp"
#include <iostream>

InputGuard::InputGuard() {
#ifdef _WIN32
    // Windows: disable line input & echo
    HANDLE hIn = GetStdHandle(STD_INPUT_HANDLE);
    GetConsoleMode(hIn, &originalMode);
    SetConsoleMode(hIn,
        originalMode & ~(ENABLE_LINE_INPUT | ENABLE_ECHO_INPUT));

    // Hide the cursor
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_CURSOR_INFO info{};
    GetConsoleCursorInfo(hOut, &info);
    originalCursorInfo = info;
    info.bVisible = FALSE;
    SetConsoleCursorInfo(hOut, &info);
#else
    // POSIX: raw mode & no echo
    tcgetattr(STDIN_FILENO, &original);
    termios raw = original;
    raw.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);

    // Hide the cursor
    std::cout << "\x1b[?25l" << std::flush;
#endif
}

InputGuard::~InputGuard() {
#ifdef _WIN32
    // Restore input mode
    HANDLE hIn = GetStdHandle(STD_INPUT_HANDLE);
    SetConsoleMode(hIn, originalMode);

    // Restore cursor visibility
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    SetConsoleCursorInfo(hOut, &originalCursorInfo);
#else
    // Restore terminal settings
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &original);

    // Show the cursor again
    std::cout << "\x1b[?25h" << std::flush;
#endif
}
