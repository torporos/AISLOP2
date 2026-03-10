#include "Terminal.h"
#include <iostream>

namespace Terminal {
    // Foreground Color Definitions
    const std::string COLOR_BLACK   = "\033[30m";
    const std::string COLOR_RED     = "\033[31m";
    const std::string COLOR_GREEN   = "\033[32m";
    const std::string COLOR_YELLOW  = "\033[33m";
    const std::string COLOR_BLUE    = "\033[34m";
    const std::string COLOR_MAGENTA = "\033[35m";
    const std::string COLOR_CYAN    = "\033[36m";
    const std::string COLOR_WHITE   = "\033[37m";

    // Background Color Definitions
    const std::string BG_BLACK      = "\033[40m";
    const std::string BG_RED        = "\033[41m";
    const std::string BG_GREEN      = "\033[42m";
    const std::string BG_YELLOW     = "\033[43m";
    const std::string BG_BLUE       = "\033[44m";
    const std::string BG_MAGENTA    = "\033[45m";
    const std::string BG_CYAN       = "\033[46m";
    const std::string BG_WHITE      = "\033[47m";

    // Removed std::cout.flush() from all individual commands to prevent partial rendering.
    // The stream will naturally buffer the ANSI codes until we manually flush at the end of the frame.

    void ClearScreen() {
        std::cout << "\033[2J\033[H";
    }

    void MoveCursor(int x, int y) {
        std::cout << "\033[" << y << ";" << x << "H";
    }

    void SetColor(const std::string& foreground, const std::string& background) {
        std::cout << foreground << background;
    }

    void ResetColor() {
        std::cout << "\033[0m";
    }

    void EnterAltScreen() {
        std::cout << "\033[?1049h";
        std::cout.flush(); // Setup commands can still be flushed immediately
    }

    void ExitAltScreen() {
        std::cout << "\033[?1049l";
        std::cout.flush();
    }

    void HideCursor() {
        std::cout << "\033[?25l";
        std::cout.flush();
    }

    void ShowCursor() {
        std::cout << "\033[?25h";
        std::cout.flush();
    }
}
