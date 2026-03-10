#include "MapRenderer.h"
#include "Terminal.h"
#include <iostream>
#include <algorithm>

void MapRenderer::RenderRegion(const WorldMap& map, int target_region_id, int player_local_x, int player_local_y) {
    Terminal::ClearScreen();

    auto it = map.regions.find(target_region_id);
    if (it == map.regions.end()) return; // Failsafe
    const Region& region = it->second;

    const int viewportWidth = 70;
    const int viewportHeight = 30;

    // The region's local_grid is already exactly 70x30
    for (int y = 0; y < viewportHeight; ++y) {
        for (int x = 0; x < viewportWidth; ++x) {
            // Offset by +2 to respect the 1-based ANSI index and the top-left UI border
            Terminal::MoveCursor(x + 2, y + 2);

            // Draw player over the tile if local coordinates match
            if (x == player_local_x && y == player_local_y) {
                Terminal::SetColor(Terminal::COLOR_GREEN);
                std::cout << '@';
                continue;
            }

            const LocalTile& ltile = region.local_grid[y][x];

            if (ltile.in_region) {
                // Inside target region: render actual upscaled tile data
                Terminal::SetColor(ltile.color);
                std::cout << ltile.symbol;
            } else {
                // Outside target region: apply the organic mask
                Terminal::SetColor("\033[90m"); // Bright black / dark gray
                std::cout << '#';
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
            // Subsample the 200x200 grid to fit the viewport using scaling factors
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
                            // Edge of the viewport is also a border
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
                std::cout << '*'; 
            } else {
                Terminal::SetColor(base_format + tile.color);
                std::cout << tile.symbol;
            }
        }
    }

    Terminal::ResetColor();
    std::cout.flush();
}

void MapRenderer::RenderKingdomMap(const WorldMap& map, int target_kingdom_id, int player_macro_x, int player_macro_y) {
    Terminal::ClearScreen();

    auto it = map.kingdoms.find(target_kingdom_id);
    if (it == map.kingdoms.end()) return; // Failsafe
    const Kingdom& kingdom = it->second;

    int min_x = kingdom.min_x;
    int min_y = kingdom.min_y;

    const int viewportWidth = 70;
    const int viewportHeight = 30;

    // Loop through the 70x30 viewport mapping 1:1 using the kingdom's min coordinates as the origin
    for (int term_y = 0; term_y < viewportHeight; ++term_y) {
        for (int term_x = 0; term_x < viewportWidth; ++term_x) {
            Terminal::MoveCursor(term_x + 2, term_y + 2);

            int map_x = min_x + term_x;
            int map_y = min_y + term_y;

            // Draw player indicator at their exact macro coordinates
            if (map_x == player_macro_x && map_y == player_macro_y) {
                Terminal::SetColor(Terminal::COLOR_WHITE, "\033[42m"); // Bright @ with green background
                std::cout << '@';
                continue;
            }

            // Safety clamp to avoid segfaults if the term loops past grid limits
            if (map_x >= 0 && map_x < 200 && map_y >= 0 && map_y < 200) {
                const Tile& tile = map.grid[map_y][map_x];

                if (tile.kingdom_id == target_kingdom_id) {
                    // Color based on region ID to show internal political borders
                    std::string colors[] = {
                        Terminal::COLOR_RED, Terminal::COLOR_GREEN, 
                        Terminal::COLOR_YELLOW, Terminal::COLOR_BLUE, 
                        Terminal::COLOR_MAGENTA, Terminal::COLOR_CYAN, 
                        Terminal::COLOR_WHITE
                    };
                    Terminal::SetColor(colors[tile.region_id % 7]);
                    std::cout << tile.symbol;
                } else {
                    // Mask out other kingdoms with dark gray space
                    Terminal::SetColor("\033[90m"); 
                    std::cout << ' ';
                }
            } else {
                Terminal::SetColor("\033[90m");
                std::cout << ' ';
            }
        }
    }

    Terminal::ResetColor();
    std::cout.flush();
}
