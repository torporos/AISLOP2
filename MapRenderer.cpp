#include "MapRenderer.h"
#include "Terminal.h"
#include <iostream>
#include <algorithm>

void MapRenderer::RenderRegion(const WorldMap& map, int target_region_id, int player_x, int player_y) {
    Terminal::ClearScreen();

    // 1. Calculate the geographic center of the target region
    long long sumX = 0;
    long long sumY = 0;
    int tileCount = 0;

    for (int y = 0; y < 200; ++y) {
        for (int x = 0; x < 200; ++x) {
            if (map.grid[y][x].region_id == target_region_id) {
                sumX += x;
                sumY += y;
                tileCount++;
            }
        }
    }

    if (tileCount == 0) return; // Fail-safe if region doesn't exist

    int centerX = sumX / tileCount;
    int centerY = sumY / tileCount;

    // 2. Determine 70x30 viewport bounds
    const int viewportWidth = 70;
    const int viewportHeight = 30;

    // Clamp the starting coordinates so the viewport never exceeds the 200x200 map boundaries
    int startX = std::max(0, std::min(centerX - viewportWidth / 2, 200 - viewportWidth));
    int startY = std::max(0, std::min(centerY - viewportHeight / 2, 200 - viewportHeight));

    // 3. Render the tiles to the terminal
    for (int y = 0; y < viewportHeight; ++y) {
        for (int x = 0; x < viewportWidth; ++x) {
            int mapX = startX + x;
            int mapY = startY + y;

            // Offset by +2 to respect the 1-based ANSI index and the top-left UI border
            Terminal::MoveCursor(x + 2, y + 2);

            // Draw player over the tile if coordinates match
            if (mapX == player_x && mapY == player_y) {
                Terminal::SetColor(Terminal::COLOR_GREEN);
                std::cout << '@';
            } else {
                const Tile& tile = map.grid[mapY][mapX];

                if (tile.region_id == target_region_id) {
                    // Inside target region: render actual tile data
                    Terminal::SetColor(tile.color);
                    std::cout << tile.symbol;
                } else {
                    // Outside target region: apply the organic mask
                    Terminal::SetColor("\033[90m"); // Bright black / dark gray
                    std::cout << '#';
                }
            }
        }
    }

    Terminal::ResetColor();
    std::cout.flush();
}

void MapRenderer::RenderContinentalMap(const WorldMap& map, int player_kingdom_id, int player_x, int player_y) {
    Terminal::ClearScreen();

    const int viewportWidth = 70;
    const int viewportHeight = 30;

    for (int term_y = 0; term_y < viewportHeight; ++term_y) {
        for (int term_x = 0; term_x < viewportWidth; ++term_x) {
            // Subsample the 200x200 grid to fit the viewport using the requested scaling factors
            int map_x = static_cast<int>(term_x * 2.8f);
            int map_y = static_cast<int>(term_y * 6.6f);

            // Safety bounds clamp
            if (map_x >= 200) map_x = 199;
            if (map_y >= 200) map_y = 199;

            Terminal::MoveCursor(term_x + 2, term_y + 2);

            const Tile& tile = map.grid[map_y][map_x];
            int current_kingdom = tile.kingdom_id;

            // Border Detection: Check all 8 adjacent tiles in the downsampled terminal grid
            bool is_border = false;
            if (current_kingdom != -1) {
                for (int dy = -1; dy <= 1; ++dy) {
                    for (int dx = -1; dx <= 1; ++dx) {
                        if (dx == 0 && dy == 0) continue;

                        int n_term_x = term_x + dx;
                        int n_term_y = term_y + dy;

                        // Calculate what high-res map coordinates these terminal neighbors correspond to
                        if (n_term_x >= 0 && n_term_x < viewportWidth && n_term_y >= 0 && n_term_y < viewportHeight) {
                            int n_map_x = static_cast<int>(n_term_x * 2.8f);
                            int n_map_y = static_cast<int>(n_term_y * 6.6f);

                            if (n_map_x >= 200) n_map_x = 199;
                            if (n_map_y >= 200) n_map_y = 199;

                            if (map.grid[n_map_y][n_map_x].kingdom_id != current_kingdom) {
                                is_border = true;
                                break;
                            }
                        } else {
                            // Edge of the viewport/map is also a border
                            is_border = true;
                            break;
                        }
                    }
                    if (is_border) break;
                }
            }

            // Maintain the blink code isolation logic to prevent ANSI state leaking
            std::string base_format = (current_kingdom == player_kingdom_id && current_kingdom != -1) ? "\033[5m" : "\033[25m";

            if (is_border) {
                // Determine border color based on kingdom_id
                std::string border_color;
                switch (current_kingdom) {
                    case 0: border_color = Terminal::COLOR_RED; break;
                    case 1: border_color = Terminal::COLOR_GREEN; break;
                    case 2: border_color = Terminal::COLOR_YELLOW; break;
                    case 3: border_color = Terminal::COLOR_BLUE; break;
                    case 4: border_color = Terminal::COLOR_MAGENTA; break;
                    case 5: border_color = Terminal::COLOR_CYAN; break;
                    default: border_color = Terminal::COLOR_WHITE; break;
                }

                Terminal::SetColor(base_format + border_color);
                std::cout << '*'; // Using '*' as the thick border character for reliable Cygwin rendering
            } else {
                // Not a border (or unclaimed land): print the biome symbol and color
                Terminal::SetColor(base_format + tile.color);
                std::cout << tile.symbol;
            }
        }
    }

    Terminal::ResetColor();
    std::cout.flush();
}

void MapRenderer::RenderKingdomMap(const WorldMap& map, int target_kingdom_id, int player_region_id) {
    Terminal::ClearScreen();

    // 1. Find the 'capital' (geographic center) of the target kingdom
    long long sumX = 0;
    long long sumY = 0;
    int tileCount = 0;

    for (int y = 0; y < 200; ++y) {
        for (int x = 0; x < 200; ++x) {
            if (map.grid[y][x].kingdom_id == target_kingdom_id) {
                sumX += x;
                sumY += y;
                tileCount++;
            }
        }
    }

    if (tileCount == 0) return; // Failsafe if kingdom not found

    int centerX = sumX / tileCount;
    int centerY = sumY / tileCount;

    // 2. Center a 70x30 viewport exactly on those coordinates within the 200x200 grid
    const int viewportWidth = 70;
    const int viewportHeight = 30;

    int startX = std::max(0, std::min(centerX - viewportWidth / 2, 200 - viewportWidth));
    int startY = std::max(0, std::min(centerY - viewportHeight / 2, 200 - viewportHeight));

    // Pre-calculate the geographical center of the player's region to place the '@'
    long long pr_sum_x = 0, pr_sum_y = 0;
    int pr_count = 0;

    // Only search if a valid region ID is passed
    if (player_region_id != -1) {
        for (int y = 0; y < 200; ++y) {
            for (int x = 0; x < 200; ++x) {
                if (map.grid[y][x].region_id == player_region_id) {
                    pr_sum_x += x;
                    pr_sum_y += y;
                    pr_count++;
                }
            }
        }
    }

    int pr_center_x = (pr_count > 0) ? (pr_sum_x / pr_count) : -1;
    int pr_center_y = (pr_count > 0) ? (pr_sum_y / pr_count) : -1;

    // 3. Loop through the 70x30 viewport
    for (int term_y = 0; term_y < viewportHeight; ++term_y) {
        for (int term_x = 0; term_x < viewportWidth; ++term_x) {
            Terminal::MoveCursor(term_x + 2, term_y + 2);

            int map_x = startX + term_x;
            int map_y = startY + term_y;

            // Safety clamps
            if (map_x > 199) map_x = 199;
            if (map_y > 199) map_y = 199;

            const Tile& tile = map.grid[map_y][map_x];

            // Explicitly prevent blink leaking into this view
            std::string base_format = "\033[25m";

            if (map_x == pr_center_x && map_y == pr_center_y) {
                // Draw player indicator at the calculated center
                Terminal::SetColor(base_format + Terminal::COLOR_WHITE, "\033[42m"); // Bright @ with green background
                std::cout << '@';
            } else if (tile.kingdom_id != target_kingdom_id) {
                // Mask out neighboring kingdoms with dark gray space
                Terminal::SetColor(base_format + "\033[90m");
                std::cout << ' ';
            } else {
                // Inside target kingdom: Region Border Detection
                bool is_region_border = false;

                // Only check region borders if regions have been generated (region_id != -1)
                if (tile.region_id != -1) {
                    int dx[] = {0, 0, 1, -1};
                    int dy[] = {1, -1, 0, 0};
                    for (int i = 0; i < 4; ++i) {
                        int nx = map_x + dx[i];
                        int ny = map_y + dy[i];

                        if (nx >= 0 && nx < 200 && ny >= 0 && ny < 200) {
                            if (map.grid[ny][nx].region_id != tile.region_id) {
                                is_region_border = true;
                                break;
                            }
                        }
                    }
                }

                if (is_region_border) {
                    // Invert the ANSI colors (\033[7m) to draw internal regional lines
                    Terminal::SetColor(base_format + "\033[7m" + tile.color);
                    std::cout << tile.symbol;
                } else {
                    // Print the tile's base Biome symbol
                    Terminal::SetColor(base_format + tile.color);
                    std::cout << tile.symbol;
                }
            }
        }
    }

    Terminal::ResetColor();
    std::cout.flush();
}
