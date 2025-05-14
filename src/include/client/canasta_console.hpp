
#ifndef CANASTA_CONSOLE_HPP
#define CANASTA_CONSOLE_HPP

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif

#include <string>

/**
 * @class CanastaConsole
 * @brief A class for console output with color support and UTF-8 encoding.
 */
class CanastaConsole {
public:

    /**
     * @enum Color
     * @brief Enum representing the color options for console output.
     */
    enum class Color {
        Default, Red, Green, Yellow,
        Blue, Magenta, Cyan, White,
        BrightRed, BrightGreen, BrightYellow,
        BrightBlue, BrightMagenta, BrightCyan, BrightWhite
    };
    /// Constructor: enables UTF-8 + ANSI on Windows
    CanastaConsole();

    /**
     * @brief Print a message to the console with optional color and newline.
     */
    void print(const std::string& msg,
                Color color   = Color::Default,
                bool newline  = false);

    void printNewLine();
    void printSpace(std::size_t count = 1);

    /// Clear the console screenme
    void clear();

private:
    /// Wrap text in ANSI escape codes for the given color
    static std::string applyColor(const std::string& text, Color color);
};
#endif //CANASTA_CONSOLE_HPP
