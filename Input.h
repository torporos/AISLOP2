#ifndef INPUT_H
#define INPUT_H

#include <termios.h>
#include <unistd.h>
#include <stdio.h>

namespace Input {
    // Reads a single character from standard input without waiting for the Enter key.
    // Uses POSIX termios to temporarily disable canonical mode and echo.
    inline char GetCharWithoutEnter() {
        struct termios oldt, newt;
        char ch;

        // Get current terminal attributes
        tcgetattr(STDIN_FILENO, &oldt);
        newt = oldt;

        // Disable canonical mode (line-by-line input) and echo (displaying typed chars)
        newt.c_lflag &= ~(ICANON | ECHO);

        // Set the new attributes immediately
        tcsetattr(STDIN_FILENO, TCSANOW, &newt);

        // Read a single character
        ch = getchar();

        // Restore original terminal attributes
        tcsetattr(STDIN_FILENO, TCSANOW, &oldt);

        return ch;
    }
}

#endif // INPUT_H
