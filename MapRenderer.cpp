#include "MapRenderer.h"
#include "Terminal.h"
#include <iostream>
#include <algorithm>

void MapRenderer::RenderRegion(const WorldMap& map, int target_region_id, int player_local_x, int player_local_y) {
    Terminal::ClearScreen();

    auto it = map.regions.find(target_region_id);
    if (it == map.regions.end()) return; 
    const Region& region = it->second;

    const int viewportWidth = 70;
    const int viewportHeight = 30;

    std::string base_format = "\033[0m"; // Guarantee no color bleed

    for (int y = 0; y < viewportHeight; ++y) {
        for (int x = 0; x < viewportWidth; ++x) {
            Terminal::MoveCursor(x + 2, y + 2);

            if (x == player_local_x && y == player_local_y) {
                Terminal::SetColor(base_format + Terminal::COLOR_GREEN);
                std::cout << '@';
                continue;
            }

            const LocalTile& ltile = region.local_grid[y][x];

            if (ltile.in_region) {
                Terminal::SetColor(base_format + ltile.color);
                std::cout << ltile.symbol;
            } else {
                Terminal::SetColor(base_format + "\033[90m");
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
            int map_x = static_cast<int>(term_x * 2.8f);
            int map_y = static_cast<int>(term_y * 6.6f);

            if (map_x >= 200) map_x = 199;
            if (map_y >= 200) map_y = 199;

            Terminal::MoveCursor(term_x + 2, term_y + 2);

            const Tile& tile = map.grid[map_y][map_x];
            int current_kingdom = tile.kingdom_id;

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
                            is_border = true;
                            break;
                        }
                    }
                    if (is_border) break;
                }
            }

            // Always embed \033[0m to ensure attributes are stripped
            std::string base_format = (current_kingdom == player_kingdom_id && current_kingdom != -1) ? "\033[0m\033[5m" : "\033[0m";

            if (is_border) {
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
    if (it == map.kingdoms.end()) return; 
    const Kingdom& kingdom = it->second;

    int min_x = kingdom.min_x;
    int min_y = kingdom.min_y;

    int player_region_id = -1;
    if (player_macro_x >= 0 && player_macro_x < 200 && player_macro_y >= 0 && player_macro_y < 200) {
        player_region_id = map.grid[player_macro_y][player_macro_x].region_id;
    }

    const int viewportWidth = 70;
    const int viewportHeight = 30;

    for (int term_y = 0; term_y < viewportHeight; ++term_y) {
        for (int term_x = 0; term_x < viewportWidth; ++term_x) {
            Terminal::MoveCursor(term_x + 2, term_y + 2);

            int map_x = min_x + term_x;
            int map_y = min_y + term_y;

            // Strict ANSI wipe to eliminate background color bleed on subsequent iterations
            std::string base_format = "\033[0m";

            if (map_x == player_macro_x && map_y == player_macro_y) {
                Terminal::SetColor(base_format + Terminal::COLOR_WHITE, "\033[42m"); 
                std::cout << '@';
                continue;
            }

            if (map_x >= 0 && map_x < 200 && map_y >= 0 && map_y < 200) {
                const Tile& tile = map.grid[map_y][map_x];

                if (tile.kingdom_id == target_kingdom_id) {
                    bool is_region_border = false;
                    
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

                    // Dynamically highlight the specific region the player is inside
                    std::string bg_format = "";
                    if (tile.region_id == player_region_id && player_region_id != -1) {
                        bg_format = "\033[100m"; // Dark Gray Background
                    }

                    std::string colors[] = {
                        Terminal::COLOR_RED, Terminal::COLOR_GREEN, 
                        Terminal::COLOR_YELLOW, Terminal::COLOR_BLUE, 
                        Terminal::COLOR_MAGENTA, Terminal::COLOR_CYAN, 
                        Terminal::COLOR_WHITE
                    };
                    std::string region_color = colors[tile.region_id % 7];

                    if (is_region_border) {
                        Terminal::SetColor(base_format + "\033[7m" + region_color, bg_format);
                        std::cout << tile.symbol;
                    } else {
                        Terminal::SetColor(base_format + region_color, bg_format);
                        std::cout << tile.symbol;
                    }
                } else {
                    Terminal::SetColor(base_format + "\033[90m", ""); 
                    std::cout << ' ';
                }
            } else {
                Terminal::SetColor(base_format + "\033[90m", "");
                std::cout << ' ';
            }
        }
    }

    Terminal::ResetColor();
    std::cout.flush();
}
