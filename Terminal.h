#ifndef TERMINAL_H
#define TERMINAL_H

#include <string>

namespace Terminal {
    // Foreground Colors
    extern const std::string COLOR_BLACK;
    extern const std::string COLOR_RED;
    extern const std::string COLOR_GREEN;
    extern const std::string COLOR_YELLOW;
    extern const std::string COLOR_BLUE;
    extern const std::string COLOR_MAGENTA;
    extern const std::string COLOR_CYAN;
    extern const std::string COLOR_WHITE;

    // Background Colors
    extern const std::string BG_BLACK;
    extern const std::string BG_RED;
    extern const std::string BG_GREEN;
    extern const std::string BG_YELLOW;
    extern const std::string BG_BLUE;
    extern const std::string BG_MAGENTA;
    extern const std::string BG_CYAN;
    extern const std::string BG_WHITE;

    // Terminal Manipulation Functions
    void ClearScreen();
    void MoveCursor(int x, int y);
    void SetColor(const std::string& foreground, const std::string& background = "");
    void ResetColor();

    // Terminal Lifecycle Functions
    void EnterAltScreen();
    void ExitAltScreen();
    void HideCursor();
    void ShowCursor();
}

#endif // TERMINAL_H
