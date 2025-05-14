// input_guard.hpp
#ifndef INPUT_GUARD_HPP
#define INPUT_GUARD_HPP

#ifdef _WIN32
#include <windows.h>
#else
#include <termios.h>
#include <unistd.h>
#endif

/**
 * @class InputGuard
 * @brief RAII guard: on construction, disable line-buffering & echo; on destruction, restore.
 */
class InputGuard {
public:
    InputGuard();
    ~InputGuard();
private:
#ifdef _WIN32
    DWORD originalMode;
    CONSOLE_CURSOR_INFO  originalCursorInfo;
#else
    struct termios original;
#endif
};

#endif // INPUT_GUARD_HPP