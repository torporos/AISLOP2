#include "MapData.h"
#include "Terminal.h"
#include <random>
#include <cmath>
#include <vector>
#include <queue>
#include <algorithm>

// Helper function for bilinear interpolation
float BilinearInterpolate(float tx, float ty, float c00, float c10, float c01, float c11) {
    float a = c00 * (1.0f - tx) + c10 * tx;
    float b = c01 * (1.0f - tx) + c11 * tx;
    return a * (1.0f - ty) + b * ty;
}

// Data structure for Kingdom Dijkstra expansion
struct ExpandNode {
    float cost;
    int x;
    int y;
    int kingdom_id;

    bool operator>(const ExpandNode& other) const {
        return cost > other.cost;
    }
};

// Data structure for Region Dijkstra expansion
struct RegionExpandNode {
    float cost;
    int x;
    int y;
    int region_id;
    int kingdom_id;

    bool operator>(const RegionExpandNode& other) const {
        return cost > other.cost;
    }
};

// Calculates the cost to expand into a specific biome based on the kingdom's race
float GetTraversalCost(int kingdom_id, Biome biome) {
    // 0: Human, 1: Elf, 2: Gnome, 3: Dwarf, 4: Troll, 5: Lizardman
    switch (kingdom_id) {
        case 0: // Human (Plains preferred)
            if (biome == Biome::Plains) return 1.0f;
            if (biome == Biome::Forest) return 2.0f;
            if (biome == Biome::Hills) return 3.0f;
            if (biome == Biome::Swamp) return 5.0f;
            if (biome == Biome::Desert) return 5.0f;
            if (biome == Biome::Mountains) return 8.0f;
            break;
        case 1: // Elf (Forest preferred)
            if (biome == Biome::Forest) return 1.0f;
            if (biome == Biome::Plains) return 2.0f;
            if (biome == Biome::Hills) return 4.0f;
            if (biome == Biome::Swamp) return 5.0f;
            if (biome == Biome::Mountains) return 8.0f;
            if (biome == Biome::Desert) return 10.0f;
            break;
        case 2: // Gnome (Hills preferred)
            if (biome == Biome::Hills) return 1.0f;
            if (biome == Biome::Forest) return 2.0f;
            if (biome == Biome::Plains) return 3.0f;
            if (biome == Biome::Mountains) return 4.0f;
            if (biome == Biome::Swamp) return 6.0f;
            if (biome == Biome::Desert) return 8.0f;
            break;
        case 3: // Dwarf (Mountains preferred)
            if (biome == Biome::Mountains) return 1.0f;
            if (biome == Biome::Hills) return 2.0f;
            if (biome == Biome::Plains) return 5.0f;
            if (biome == Biome::Forest) return 8.0f;
            if (biome == Biome::Desert) return 8.0f;
            if (biome == Biome::Swamp) return 10.0f;
            break;
        case 4: // Troll (Swamp preferred)
            if (biome == Biome::Swamp) return 1.0f;
            if (biome == Biome::Forest) return 3.0f;
            if (biome == Biome::Plains) return 5.0f;
            if (biome == Biome::Hills) return 8.0f;
            if (biome == Biome::Mountains) return 10.0f;
            if (biome == Biome::Desert) return 10.0f;
            break;
        case 5: // Lizardman (Desert preferred)
            if (biome == Biome::Desert) return 1.0f;
            if (biome == Biome::Plains) return 3.0f;
            if (biome == Biome::Hills) return 5.0f;
            if (biome == Biome::Swamp) return 5.0f;
            if (biome == Biome::Forest) return 8.0f;
            if (biome == Biome::Mountains) return 10.0f;
            break;
    }
    return 10.0f; // Fallback cost
}

void WorldMap::GenerateWorld(unsigned int seed) {
    // Initialize standard C++ random number generator
    std::mt19937 gen(seed);
    std::uniform_real_distribution<float> dist(0.0f, 1.0f);
    std::uniform_int_distribution<int> coord_dist(0, 199);

    // Grid size for the low-resolution noise (20x20, plus 1 for boundary interpolation)
    const int noiseSize = 21;
    std::vector<std::vector<float>> lowResElev(noiseSize, std::vector<float>(noiseSize));
    std::vector<std::vector<float>> lowResMoist(noiseSize, std::vector<float>(noiseSize));

    // 1. Generate low-resolution random float grids for Elevation and Moisture
    for (int y = 0; y < noiseSize; ++y) {
        for (int x = 0; x < noiseSize; ++x) {
            lowResElev[y][x] = dist(gen);
            lowResMoist[y][x] = dist(gen);
        }
    }

    kingdoms.clear();
    regions.clear();

    // 2. Upscale to 200x200 using bilinear interpolation and assign Biomes
    for (int y = 0; y < 200; ++y) {
        for (int x = 0; x < 200; ++x) {
            float gx = x / 10.0f;
            float gy = y / 10.0f;

            int x0 = static_cast<int>(gx);
            int y0 = static_cast<int>(gy);
            int x1 = x0 + 1;
            int y1 = y0 + 1;

            float tx = gx - x0;
            float ty = gy - y0;

            float e00 = lowResElev[y0][x0];
            float e10 = lowResElev[y0][x1];
            float e01 = lowResElev[y1][x0];
            float e11 = lowResElev[y1][x1];
            float elev = BilinearInterpolate(tx, ty, e00, e10, e01, e11);

            float m00 = lowResMoist[y0][x0];
            float m10 = lowResMoist[y0][x1];
            float m01 = lowResMoist[y1][x0];
            float m11 = lowResMoist[y1][x1];
            float moist = BilinearInterpolate(tx, ty, m00, m10, m01, m11);

            Tile& tile = grid[y][x];
            tile.kingdom_id = -1; // Unclaimed
            tile.region_id = -1;
            tile.is_road = false;

            // 3. Assign Biome
            if (elev < 0.25f) {
                tile.biome = Biome::Desert;
                tile.symbol = '-';
                tile.color = "\033[93m";
            } else if (elev >= 0.65f) {
                if (moist < 0.5f) {
                    tile.biome = Biome::Mountains;
                    tile.symbol = '^';
                    tile.color = Terminal::COLOR_WHITE;
                } else {
                    tile.biome = Biome::Hills;
                    tile.symbol = 'n';
                    tile.color = Terminal::COLOR_YELLOW;
                }
            } else {
                if (moist >= 0.66f) {
                    tile.biome = Biome::Swamp;
                    tile.symbol = '~';
                    tile.color = Terminal::COLOR_GREEN;
                } else if (moist >= 0.33f) {
                    tile.biome = Biome::Forest;
                    tile.symbol = 'T';
                    tile.color = "\033[92m";
                } else {
                    tile.biome = Biome::Plains;
                    tile.symbol = '.';
                    tile.color = Terminal::COLOR_YELLOW;
                }
            }
        }
    }

    // 4. Kingdom Seeding
    std::priority_queue<ExpandNode, std::vector<ExpandNode>, std::greater<ExpandNode>> pq;
    std::vector<std::vector<float>> min_cost(200, std::vector<float>(200, 1e9f));

    Biome preferredBiomes[6] = {
        Biome::Plains, Biome::Forest, Biome::Hills, 
        Biome::Mountains, Biome::Swamp, Biome::Desert
    };

    for (int i = 0; i < 6; ++i) {
        int sx, sy;
        while (true) {
            sx = coord_dist(gen);
            sy = coord_dist(gen);

            if (grid[sy][sx].biome == preferredBiomes[i] && grid[sy][sx].kingdom_id == -1) {
                break;
            }
        }

        Kingdom k;
        k.id = i;
        k.primary_biome = preferredBiomes[i];
        
        // Initialize bounding box perfectly around the seed
        k.min_x = sx;
        k.max_x = sx;
        k.min_y = sy;
        k.max_y = sy;
        
        kingdoms[i] = k;

        pq.push({0.0f, sx, sy, i});
        min_cost[sy][sx] = 0.0f;
    }

    // 5. Multi-source Dijkstra Expansion for Kingdoms (with Bounding Box constraints)
    int dx[] = {0, 0, 1, -1};
    int dy[] = {1, -1, 0, 0};

    while (!pq.empty()) {
        ExpandNode current = pq.top();
        pq.pop();

        if (grid[current.y][current.x].kingdom_id != -1) {
            continue;
        }

        Kingdom& k_data = kingdoms[current.kingdom_id];
        
        // Finalize BB update
        k_data.min_x = std::min(k_data.min_x, current.x);
        k_data.max_x = std::max(k_data.max_x, current.x);
        k_data.min_y = std::min(k_data.min_y, current.y);
        k_data.max_y = std::max(k_data.max_y, current.y);

        grid[current.y][current.x].kingdom_id = current.kingdom_id;

        for (int i = 0; i < 4; ++i) {
            int nx = current.x + dx[i];
            int ny = current.y + dy[i];

            if (nx >= 0 && nx < 200 && ny >= 0 && ny < 200) {
                if (grid[ny][nx].kingdom_id == -1) {
                    
                    // Strict Viewport Constraint: Check BB before allowing expansion
                    int proposed_min_x = std::min(k_data.min_x, nx);
                    int proposed_max_x = std::max(k_data.max_x, nx);
                    int proposed_min_y = std::min(k_data.min_y, ny);
                    int proposed_max_y = std::max(k_data.max_y, ny);

                    if ((proposed_max_x - proposed_min_x) > 68 || (proposed_max_y - proposed_min_y) > 28) {
                        continue; // Skip, would cause kingdom to outgrow a 70x30 viewport
                    }

                    float step_cost = GetTraversalCost(current.kingdom_id, grid[ny][nx].biome);
                    float total_cost = current.cost + step_cost;

                    if (total_cost < min_cost[ny][nx]) {
                        min_cost[ny][nx] = total_cost;
                        pq.push({total_cost, nx, ny, current.kingdom_id});
                    }
                }
            }
        }
    }

    // 6. Proportional Region Seeding
    std::vector<int> kingdom_tile_counts(6, 0);
    for (int y = 0; y < 200; ++y) {
        for (int x = 0; x < 200; ++x) {
            if (grid[y][x].kingdom_id != -1) {
                kingdom_tile_counts[grid[y][x].kingdom_id]++;
            }
        }
    }

    std::priority_queue<RegionExpandNode, std::vector<RegionExpandNode>, std::greater<RegionExpandNode>> reg_pq;
    std::vector<std::vector<float>> reg_min_cost(200, std::vector<float>(200, 1e9f));

    int next_region_id = 0;
    for (int k = 0; k < 6; ++k) {
        int count = kingdom_tile_counts[k];
        int num_regions = std::max(12, count / 40);

        for (int r = 0; r < num_regions; ++r) {
            int rx, ry;
            bool valid = false;
            int attempts = 0;
            
            // Seed strictly inside kingdom bounds
            while (!valid && attempts < 1000) {
                rx = coord_dist(gen);
                ry = coord_dist(gen);
                if (grid[ry][rx].kingdom_id == k) {
                    valid = true;
                }
                attempts++;
            }

            if (!valid) continue;

            Region region;
            region.id = next_region_id;
            region.kingdom_id = k;
            regions[next_region_id] = region;

            kingdoms[k].region_ids.push_back(next_region_id);

            reg_pq.push({0.0f, rx, ry, next_region_id, k});
            reg_min_cost[ry][rx] = 0.0f;
            next_region_id++;
        }
    }

    // 7. Organic Region Expansion (Second Dijkstra Pass)
    while (!reg_pq.empty()) {
        auto current = reg_pq.top();
        reg_pq.pop();

        if (grid[current.y][current.x].region_id != -1) {
            continue;
        }

        grid[current.y][current.x].region_id = current.region_id;

        for (int i = 0; i < 4; ++i) {
            int nx = current.x + dx[i];
            int ny = current.y + dy[i];

            if (nx >= 0 && nx < 200 && ny >= 0 && ny < 200) {
                // Must belong to the exact same parent kingdom to prevent borders leaking
                if (grid[ny][nx].kingdom_id == current.kingdom_id && grid[ny][nx].region_id == -1) {
                    float step_cost = GetTraversalCost(current.kingdom_id, grid[ny][nx].biome);
                    float total_cost = current.cost + step_cost;

                    if (total_cost < reg_min_cost[ny][nx]) {
                        reg_min_cost[ny][nx] = total_cost;
                        reg_pq.push({total_cost, nx, ny, current.region_id, current.kingdom_id});
                    }
                }
            }
        }
    }

    // 8. Geographic Features generation (Rivers and Ridges on region borders)
    std::uniform_real_distribution<float> feature_chance(0.0f, 1.0f);
    std::uniform_int_distribution<int> feature_type(0, 1);

    for (int y = 1; y < 199; ++y) {
        for (int x = 1; x < 199; ++x) {
            int current_region = grid[y][x].region_id;
            
            if (current_region == -1) continue;

            bool is_border = (grid[y-1][x].region_id != current_region ||
                              grid[y+1][x].region_id != current_region ||
                              grid[y][x-1].region_id != current_region ||
                              grid[y][x+1].region_id != current_region);

            if (is_border) {
                if (feature_chance(gen) < 0.30f) {
                    if (feature_type(gen) == 0) {
                        grid[y][x].symbol = '~';
                        grid[y][x].color = Terminal::COLOR_BLUE;
                    } else {
                        grid[y][x].symbol = '^';
                        grid[y][x].color = "\033[90m"; // Dark gray
                    }
                }
            }
        }
    }

    // 9. POI Placement (using macro coordinates)
    std::uniform_int_distribution<int> poi_count_dist(2, 4);
    int next_poi_id = 0;

    for (auto& pair : regions) {
        Region& region = pair.second;
        int num_pois = poi_count_dist(gen);

        for (int i = 0; i < num_pois; ++i) {
            int px, py;
            bool valid = false;
            int attempts = 0;

            while (!valid && attempts < 1000) {
                px = coord_dist(gen);
                py = coord_dist(gen);
                if (grid[py][px].region_id == region.id && !grid[py][px].is_road && 
                    grid[py][px].symbol != 'O' && grid[py][px].symbol != 'V') {
                    valid = true;
                }
                attempts++;
            }

            if (!valid) continue;

            POI new_poi;
            new_poi.id = next_poi_id++;
            new_poi.macro_x = px;
            new_poi.macro_y = py;
            new_poi.local_x = 0; // Initialize high-res coordinates to defaults for now
            new_poi.local_y = 0;

            if (i == 0) {
                new_poi.symbol = 'O'; // Settlement
                grid[py][px].symbol = 'O';
                grid[py][px].color = Terminal::COLOR_WHITE;
            } else {
                new_poi.symbol = 'V'; // Dungeon
                grid[py][px].symbol = 'V';
                grid[py][px].color = Terminal::COLOR_RED;
            }

            region.pois.push_back(new_poi);
        }

        // 10. Road Generation (Minimum Spanning Tree with macro coordinates)
        if (region.pois.size() >= 2) {
            std::vector<size_t> connected;
            std::vector<size_t> unconnected;
            
            connected.push_back(0);
            for (size_t i = 1; i < region.pois.size(); ++i) {
                unconnected.push_back(i);
            }

            std::vector<std::pair<size_t, size_t>> edges;

            while (!unconnected.empty()) {
                float min_dist = 1e9f;
                size_t best_c = 0;
                size_t best_u_idx = 0;

                for (size_t c : connected) {
                    for (size_t u_idx = 0; u_idx < unconnected.size(); ++u_idx) {
                        size_t u = unconnected[u_idx];
                        float local_dx = static_cast<float>(region.pois[c].macro_x - region.pois[u].macro_x);
                        float local_dy = static_cast<float>(region.pois[c].macro_y - region.pois[u].macro_y);
                        float dist_val = std::sqrt(local_dx * local_dx + local_dy * local_dy);
                        
                        if (dist_val < min_dist) {
                            min_dist = dist_val;
                            best_c = c;
                            best_u_idx = u_idx;
                        }
                    }
                }

                size_t best_u = unconnected[best_u_idx];
                edges.push_back({best_c, best_u});
                connected.push_back(best_u);
                unconnected.erase(unconnected.begin() + best_u_idx);
            }

            for (const auto& edge : edges) {
                POI& start = region.pois[edge.first];
                POI& end = region.pois[edge.second];

                int curX = start.macro_x;
                int curY = start.macro_y;

                auto drawRoad = [&](int x, int y) {
                    if (grid[y][x].symbol != 'O' && grid[y][x].symbol != 'V') {
                        grid[y][x].symbol = '+';
                        grid[y][x].color = Terminal::COLOR_YELLOW;
                        grid[y][x].is_road = true;
                    }
                };

                while (curX != end.macro_x) {
                    curX += (end.macro_x > curX) ? 1 : -1;
                    drawRoad(curX, curY);
                }
                while (curY != end.macro_y) {
                    curY += (end.macro_y > curY) ? 1 : -1;
                    drawRoad(curX, curY);
                }

                start.connected_pois.push_back(end.id);
                end.connected_pois.push_back(start.id);
            }
        }
    }
}
