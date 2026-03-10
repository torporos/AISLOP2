#ifndef RENDERSYSTEM_H
#define RENDERSYSTEM_H

#include "ECS_Types.h"
#include "Terminal.h"
#include <vector>
#include <string>
#include <array>

class Coordinator;

// Represents a single block on the terminal grid
struct ConsoleCell {
    char symbol = ' ';
    std::string color = Terminal::COLOR_WHITE;

    bool operator!=(const ConsoleCell& other) const {
        return symbol != other.symbol || color != other.color;
    }
};

class RenderSystem {
public:
    std::vector<Entity> mEntities;

    RenderSystem();

    void Update(Coordinator& coordinator);

private:
    static const int VIEWPORT_WIDTH = 70;
    static const int VIEWPORT_HEIGHT = 30;

    // Include the border natively in the buffer dimensions
    static const int BUFFER_WIDTH = VIEWPORT_WIDTH + 2;
    static const int BUFFER_HEIGHT = VIEWPORT_HEIGHT + 2;

    // Double buffering arrays
    std::array<std::array<ConsoleCell, BUFFER_WIDTH>, BUFFER_HEIGHT> mCurrentBuffer;
    std::array<std::array<ConsoleCell, BUFFER_WIDTH>, BUFFER_HEIGHT> mNextBuffer;

    void ClearNextBuffer();
    void DrawBordersToNextBuffer();
};

#endif // RENDERSYSTEM_H
