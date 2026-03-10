#include "RenderSystem.h"
#include "Coordinator.h"
#include "Components.h"
#include "Terminal.h"
#include <iostream>

RenderSystem::RenderSystem() {
    ClearNextBuffer();

    // Initialize the current buffer with null characters.
    // This guarantees that the first frame will be entirely different from the current buffer,
    // forcing the system to draw the entire viewport once.
    for(int y = 0; y < BUFFER_HEIGHT; ++y) {
        for(int x = 0; x < BUFFER_WIDTH; ++x) {
            mCurrentBuffer[y][x].symbol = '\0';
            mCurrentBuffer[y][x].color = "";
        }
    }
}

void RenderSystem::ClearNextBuffer() {
    for(int y = 0; y < BUFFER_HEIGHT; ++y) {
        for(int x = 0; x < BUFFER_WIDTH; ++x) {
            mNextBuffer[y][x].symbol = ' ';
            mNextBuffer[y][x].color = Terminal::COLOR_WHITE;
        }
    }
}

void RenderSystem::DrawBordersToNextBuffer() {
    // Draw Top and Bottom
    for (int x = 0; x < BUFFER_WIDTH; ++x) {
        mNextBuffer[0][x].symbol = '-';
        mNextBuffer[BUFFER_HEIGHT - 1][x].symbol = '-';
    }
    // Draw Left and Right
    for (int y = 0; y < BUFFER_HEIGHT; ++y) {
        mNextBuffer[y][0].symbol = '|';
        mNextBuffer[y][BUFFER_WIDTH - 1].symbol = '|';
    }
    // Draw Corners
    mNextBuffer[0][0].symbol = '+';
    mNextBuffer[0][BUFFER_WIDTH - 1].symbol = '+';
    mNextBuffer[BUFFER_HEIGHT - 1][0].symbol = '+';
    mNextBuffer[BUFFER_HEIGHT - 1][BUFFER_WIDTH - 1].symbol = '+';
}

void RenderSystem::Update(Coordinator& coordinator) {
    // 1. Prepare the logical frame
    ClearNextBuffer();
    DrawBordersToNextBuffer();

    // 2. Map ECS components into the buffer
    for (auto const& entity : mEntities) {
        auto const& position = coordinator.GetComponent<Position>(entity);
        auto const& renderable = coordinator.GetComponent<Renderable>(entity);

        // Calculate offset for the border (+1 because array is 0-indexed)
        int bufX = position.x + 1;
        int bufY = position.y + 1;

        // Boundary safety check
        if (bufX >= 0 && bufX < BUFFER_WIDTH && bufY >= 0 && bufY < BUFFER_HEIGHT) {
            mNextBuffer[bufY][bufX].symbol = renderable.symbol;
            mNextBuffer[bufY][bufX].color = renderable.color;
        }
    }

    // 3. Render only the differences
    std::string currentColor = "";
    bool forceColorSet = true;

    for(int y = 0; y < BUFFER_HEIGHT; ++y) {
        for(int x = 0; x < BUFFER_WIDTH; ++x) {
            // Check if this specific cell changed since the last frame
            if (mNextBuffer[y][x] != mCurrentBuffer[y][x]) {

                // ANSI escapes are 1-based, buffer is 0-based
                Terminal::MoveCursor(x + 1, y + 1);

                // Only send color ANSI codes if the color actually changed
                if (mNextBuffer[y][x].color != currentColor || forceColorSet) {
                    Terminal::SetColor(mNextBuffer[y][x].color);
                    currentColor = mNextBuffer[y][x].color;
                    forceColorSet = false;
                }

                std::cout << mNextBuffer[y][x].symbol;

                // Sync the state
                mCurrentBuffer[y][x] = mNextBuffer[y][x];
            }
        }
    }

    // 4. Reset environment and execute standard flush
    Terminal::ResetColor();
    std::cout.flush(); // One flush per frame pushes all updates to the screen instantly
}
