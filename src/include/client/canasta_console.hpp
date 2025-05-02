#ifndef CANASTA_CONSOLE_HPP
#define CANASTA_CONSOLE_HPP

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif

#include <memory>
#include <string>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <fmt/core.h>

enum class Color {
    Default, Red, Green, Yellow,
    Blue, Magenta, Cyan, White,
    BrightRed, BrightGreen, BrightYellow,
    BrightBlue, BrightMagenta, BrightCyan, BrightWhite
};


/*class CanastaConsole {
public:
    CanastaConsole();
    void print(const std::string& message,
        Color color = Color::Default,
        bool newline = true);
    void clear();

private:
    std::shared_ptr<spdlog::logger> loggerWithNL;
    std::shared_ptr<spdlog::logger> loggerNoNL;

    static std::string applyColor(const std::string& msg, Color c);

    template<typename BaseSink>
    class SinkNoNewline : public BaseSink {
    protected:
        using mutex_t = typename BaseSink::mutex_t;
        void sink_it(const spdlog::details::log_msg &msg) override {
            spdlog::memory_buf_t formatted;
            this->formatter_->format(msg, formatted);
            std::lock_guard<mutex_t> lock(this->mutex_);
            BaseSink::stream() << fmt::to_string(formatted);
            BaseSink::flush_();
        }
    };
};*/

class CanastaConsole {
public:
    // Constructor: enables UTF-8 + ANSI on Windows
    CanastaConsole();

    // Print a message with optional color and trailing newline
    void print(const std::string& msg,
                Color color   = Color::Default,
                bool newline  = true);

    // Clear the console screen and move cursor to home
    void clear();

private:
    // Wrap text in ANSI escape codes for the given color
    static std::string applyColor(const std::string& text, Color color);
};

#endif // CANASTA_CONSOLE_HPP