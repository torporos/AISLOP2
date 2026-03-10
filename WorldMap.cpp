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

// Data structures for Dijkstra expansions
struct ExpandNode {
    float cost;
    int x;
    int y;
    int kingdom_id;

    bool operator>(const ExpandNode& other) const { return cost > other.cost; }
};

struct RegionExpandNode {
    float cost;
    int x;
    int y;
    int region_id;
    int kingdom_id;

    bool operator>(const RegionExpandNode& other) const { return cost > other.cost; }
};

struct RoadNode {
    float cost;
    int x;
    int y;

    bool operator>(const RoadNode& other) const { return cost > other.cost; }
};
struct Point { int x, y; };

float GetTraversalCost(int kingdom_id, Biome biome) {
    switch (kingdom_id) {
        case 0: // Human
            if (biome == Biome::Plains) return 1.0f;
            if (biome == Biome::Forest) return 2.0f;
            if (biome == Biome::Hills) return 3.0f;
            if (biome == Biome::Swamp) return 5.0f;
            if (biome == Biome::Desert) return 5.0f;
            if (biome == Biome::Mountains) return 8.0f;
            break;
        case 1: // Elf
            if (biome == Biome::Forest) return 1.0f;
            if (biome == Biome::Plains) return 2.0f;
            if (biome == Biome::Hills) return 4.0f;
            if (biome == Biome::Swamp) return 5.0f;
            if (biome == Biome::Mountains) return 8.0f;
            if (biome == Biome::Desert) return 10.0f;
            break;
        case 2: // Gnome
            if (biome == Biome::Hills) return 1.0f;
            if (biome == Biome::Forest) return 2.0f;
            if (biome == Biome::Plains) return 3.0f;
            if (biome == Biome::Mountains) return 4.0f;
            if (biome == Biome::Swamp) return 6.0f;
            if (biome == Biome::Desert) return 8.0f;
            break;
        case 3: // Dwarf
            if (biome == Biome::Mountains) return 1.0f;
            if (biome == Biome::Hills) return 2.0f;
            if (biome == Biome::Plains) return 5.0f;
            if (biome == Biome::Forest) return 8.0f;
            if (biome == Biome::Desert) return 8.0f;
            if (biome == Biome::Swamp) return 10.0f;
            break;
        case 4: // Troll
            if (biome == Biome::Swamp) return 1.0f;
            if (biome == Biome::Forest) return 3.0f;
            if (biome == Biome::Plains) return 5.0f;
            if (biome == Biome::Hills) return 8.0f;
            if (biome == Biome::Mountains) return 10.0f;
            if (biome == Biome::Desert) return 10.0f;
            break;
        case 5: // Lizardman
            if (biome == Biome::Desert) return 1.0f;
            if (biome == Biome::Plains) return 3.0f;
            if (biome == Biome::Hills) return 5.0f;
            if (biome == Biome::Swamp) return 5.0f;
            if (biome == Biome::Forest) return 8.0f;
            if (biome == Biome::Mountains) return 10.0f;
            break;
    }
    return 10.0f; 
}

void WorldMap::GenerateWorld(unsigned int seed) {
    std::mt19937 gen(seed);
    std::uniform_real_distribution<float> dist(0.0f, 1.0f);
    std::uniform_int_distribution<int> coord_dist(0, 199);

    const int noiseSize = 21;
    std::vector<std::vector<float>> lowResElev(noiseSize, std::vector<float>(noiseSize));
    std::vector<std::vector<float>> lowResMoist(noiseSize, std::vector<float>(noiseSize));

    for (int y = 0; y < noiseSize; ++y) {
        for (int x = 0; x < noiseSize; ++x) {
            lowResElev[y][x] = dist(gen);
            lowResMoist[y][x] = dist(gen);
        }
    }

    kingdoms.clear();
    regions.clear();

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
            tile.kingdom_id = -1;
            tile.region_id = -1;
            tile.is_road = false;

            if (elev < 0.25f) {
                tile.biome = Biome::Desert;
                tile.symbol = "-";
                tile.color = "\033[93m";
            } else if (elev >= 0.65f) {
                if (moist < 0.5f) {
                    tile.biome = Biome::Mountains;
                    tile.symbol = "^";
                    tile.color = Terminal::COLOR_WHITE;
                } else {
                    tile.biome = Biome::Hills;
                    tile.symbol = "n";
                    tile.color = Terminal::COLOR_YELLOW;
                }
            } else {
                if (moist >= 0.66f) {
                    tile.biome = Biome::Swamp;
                    tile.symbol = "~";
                    tile.color = Terminal::COLOR_GREEN;
                } else if (moist >= 0.33f) {
                    tile.biome = Biome::Forest;
                    tile.symbol = "T";
                    tile.color = "\033[92m";
                } else {
                    tile.biome = Biome::Plains;
                    tile.symbol = ".";
                    tile.color = Terminal::COLOR_YELLOW;
                }
            }
        }
    }

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
        k.min_x = sx;
        k.max_x = sx;
        k.min_y = sy;
        k.max_y = sy;
        
        kingdoms[i] = k;

        pq.push({0.0f, sx, sy, i});
        min_cost[sy][sx] = 0.0f;
    }

    int dx[] = {0, 0, 1, -1};
    int dy[] = {1, -1, 0, 0};

    while (!pq.empty()) {
        ExpandNode current = pq.top();
        pq.pop();

        if (grid[current.y][current.x].kingdom_id != -1) {
            continue;
        }

        Kingdom& k_data = kingdoms[current.kingdom_id];
        
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
                    
                    int proposed_min_x = std::min(k_data.min_x, nx);
                    int proposed_max_x = std::max(k_data.max_x, nx);
                    int proposed_min_y = std::min(k_data.min_y, ny);
                    int proposed_max_y = std::max(k_data.max_y, ny);

                    if ((proposed_max_x - proposed_min_x) > 68 || (proposed_max_y - proposed_min_y) > 28) {
                        continue; 
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
                        grid[y][x].symbol = "~";
                        grid[y][x].color = Terminal::COLOR_BLUE;
                    } else {
                        grid[y][x].symbol = "^";
                        grid[y][x].color = "\033[90m"; 
                    }
                }
            }
        }
    }

    // 9. Local Grid Upscaling, Brooks, POI Placement, and MST Roads
    std::uniform_int_distribution<int> poi_count_dist(2, 4);
    std::uniform_real_distribution<float> noise_dist(0.0f, 1.0f);
    std::uniform_int_distribution<int> local_x_dist(0, 69);
    std::uniform_int_distribution<int> local_y_dist(0, 29);
    
    std::uniform_int_distribution<int> brook_count_dist(2, 4);
    std::uniform_int_distribution<int> brook_len_dist(10, 25);
    std::uniform_int_distribution<int> dir_dist(-1, 1);

    int next_poi_id = 0; 

    for (auto& pair : regions) {
        Region& region = pair.second;

        // Find macro bounding box for this specific region
        int min_mx = 200, max_mx = -1;
        int min_my = 200, max_my = -1;
        
        for (int y = 0; y < 200; ++y) {
            for (int x = 0; x < 200; ++x) {
                if (grid[y][x].region_id == region.id) {
                    min_mx = std::min(min_mx, x);
                    max_mx = std::max(max_mx, x);
                    min_my = std::min(min_my, y);
                    max_my = std::max(max_my, y);
                }
            }
        }

        if (max_mx == -1) continue; 

        float scale_x = static_cast<float>(max_mx - min_mx + 1) / 70.0f;
        float scale_y = static_cast<float>(max_my - min_my + 1) / 30.0f;

        // 9a. Nearest-neighbor initial map
        std::vector<std::vector<Biome>> temp_biomes(30, std::vector<Biome>(70, Biome::Plains));

        for (int ly = 0; ly < 30; ++ly) {
            for (int lx = 0; lx < 70; ++lx) {
                int mx = min_mx + static_cast<int>(lx * scale_x);
                int my = min_my + static_cast<int>(ly * scale_y);

                mx = std::max(0, std::min(mx, 199));
                my = std::max(0, std::min(my, 199));

                LocalTile& ltile = region.local_grid[ly][lx];
                const Tile& mtile = grid[my][mx];

                if (mtile.region_id == region.id) {
                    ltile.in_region = true;
                    temp_biomes[ly][lx] = mtile.biome;
                } else {
                    ltile.in_region = false;
                }
            }
        }

        // 9b. Cellular Automata Smoothing Pass to remove blockiness
        for (int pass = 0; pass < 2; ++pass) {
            std::vector<std::vector<Biome>> pass_biomes = temp_biomes;
            for (int ly = 0; ly < 30; ++ly) {
                for (int lx = 0; lx < 70; ++lx) {
                    if (!region.local_grid[ly][lx].in_region) continue;

                    std::unordered_map<Biome, int> counts;
                    for (int sdy = -1; sdy <= 1; ++sdy) {
                        for (int sdx = -1; sdx <= 1; ++sdx) {
                            int nx = lx + sdx;
                            int ny = ly + sdy;
                            if (nx >= 0 && nx < 70 && ny >= 0 && ny < 30) {
                                if (region.local_grid[ny][nx].in_region) {
                                    counts[temp_biomes[ny][nx]]++;
                                }
                            }
                        }
                    }

                    Biome majority = temp_biomes[ly][lx];
                    int max_c = 0;
                    for (const auto& kv : counts) {
                        if (kv.second > max_c) {
                            max_c = kv.second;
                            majority = kv.first;
                        }
                    }
                    pass_biomes[ly][lx] = majority;
                }
            }
            temp_biomes = pass_biomes;
        }

        // 9c. Write smoothed biomes and apply noise
        for (int ly = 0; ly < 30; ++ly) {
            for (int lx = 0; lx < 70; ++lx) {
                LocalTile& ltile = region.local_grid[ly][lx];
                
                if (ltile.in_region) {
                    ltile.biome = temp_biomes[ly][lx];
                    ltile.is_road = false;
                    
                    switch(ltile.biome) {
                        case Biome::Plains: ltile.color = Terminal::COLOR_YELLOW; ltile.symbol = "."; break;
                        case Biome::Forest: ltile.color = "\033[92m"; ltile.symbol = "T"; break;
                        case Biome::Hills: ltile.color = Terminal::COLOR_YELLOW; ltile.symbol = "n"; break;
                        case Biome::Mountains: ltile.color = Terminal::COLOR_WHITE; ltile.symbol = "^"; break;
                        case Biome::Swamp: ltile.color = Terminal::COLOR_GREEN; ltile.symbol = "~"; break;
                        case Biome::Desert: ltile.color = "\033[93m"; ltile.symbol = "-"; break;
                    }

                    // Organic terrain detail upscaling
                    if (noise_dist(gen) < 0.3f) {
                        switch(ltile.biome) {
                            case Biome::Plains: ltile.symbol = ","; break;
                            case Biome::Forest: ltile.symbol = "t"; break;
                            case Biome::Hills: ltile.symbol = "m"; break;
                            case Biome::Mountains: ltile.symbol = "A"; break;
                            case Biome::Swamp: ltile.symbol = "="; break;
                            case Biome::Desert: ltile.symbol = "_"; break;
                        }
                    }
                } else {
                    ltile.symbol = " ";
                    ltile.color = "\033[90m";
                }
            }
        }

        // Generate Wandering Brooks
        int num_brooks = brook_count_dist(gen);
        for (int b = 0; b < num_brooks; ++b) {
            int bx = local_x_dist(gen);
            int by = local_y_dist(gen);
            int length = brook_len_dist(gen);
            
            int bias_x = dir_dist(gen);
            int bias_y = dir_dist(gen);
            if (bias_x == 0 && bias_y == 0) bias_x = 1;

            for (int l = 0; l < length; ++l) {
                if (bx >= 0 && bx < 70 && by >= 0 && by < 30) {
                    LocalTile& btile = region.local_grid[by][bx];
                    if (btile.in_region) {
                        btile.symbol = "~";
                        btile.color = "\033[96m"; // Cyan for brook to distinguish from deep swamp green
                    } else {
                        break; // Stop flowing if hitting border
                    }
                } else {
                    break; // Stop flowing if out of bounds
                }

                // Meandering flow directed towards bias
                if (noise_dist(gen) < 0.7f) {
                    bx += bias_x;
                    by += bias_y;
                } else {
                    bx += dir_dist(gen);
                    by += dir_dist(gen);
                }
            }
        }

        // Place POIs on the local grid (Avoiding water, brooks, and swamps)
        int num_pois = poi_count_dist(gen);

        for (int i = 0; i < num_pois; ++i) {
            int lx, ly;
            bool valid = false;
            int attempts = 0;

            while (!valid && attempts < 1000) {
                lx = local_x_dist(gen);
                ly = local_y_dist(gen);
                LocalTile& ltile = region.local_grid[ly][lx];

                // Strict validation to avoid placing on water elements
                if (ltile.in_region && !ltile.is_road && 
                    ltile.symbol != "O" && ltile.symbol != "V" && ltile.symbol != "~" && 
                    ltile.biome != Biome::Swamp) {
                    valid = true;
                }
                attempts++;
            }

            if (!valid) continue;

            POI new_poi;
            new_poi.id = next_poi_id++;
            new_poi.local_x = lx;
            new_poi.local_y = ly;
            new_poi.macro_x = min_mx + static_cast<int>(lx * scale_x);
            new_poi.macro_y = min_my + static_cast<int>(ly * scale_y);

            LocalTile& ltile = region.local_grid[ly][lx];

            if (i == 0) {
                new_poi.symbol = "O"; 
                ltile.symbol = "O";
                ltile.color = Terminal::COLOR_WHITE;
            } else {
                new_poi.symbol = "V"; 
                ltile.symbol = "V";
                ltile.color = Terminal::COLOR_RED;
            }

            grid[new_poi.macro_y][new_poi.macro_x].symbol = new_poi.symbol;
            grid[new_poi.macro_y][new_poi.macro_x].color = ltile.color;

            region.pois.push_back(new_poi);
        }

        // Generate Fields organically clustered around Settlement POIs
        for (const auto& new_poi : region.pois) {
            if (new_poi.symbol == "O") { 
                for (int dy = -3; dy <= 3; ++dy) {
                    for (int dx = -3; dx <= 3; ++dx) {
                        if (dx * dx + dy * dy <= 8) { 
                            int fx = new_poi.local_x + dx;
                            int fy = new_poi.local_y + dy;
                            if (fx >= 0 && fx < 70 && fy >= 0 && fy < 30) {
                                LocalTile& ftile = region.local_grid[fy][fx];
                                // Don't overwrite water/roads/POI, and favor Plains/Hills for fields
                                if (ftile.in_region && ftile.symbol != "O" && ftile.symbol != "V" && ftile.symbol != "~" && 
                                    (ftile.biome == Biome::Plains || ftile.biome == Biome::Hills)) {
                                    
                                    // Add noise to field spread so it's not a perfect circle
                                    if (noise_dist(gen) < 0.7f) {
                                        ftile.symbol = "≈";
                                        ftile.color = Terminal::COLOR_YELLOW;
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }

        // Generate Roads using bounded Dijkstra Pathfinding
        if (region.pois.size() >= 2) {
            std::vector<size_t> connected;
            std::vector<size_t> unconnected;
            
            connected.push_back(0);
            for (size_t i = 1; i < region.pois.size(); ++i) {
                unconnected.push_back(i);
            }

            std::vector<std::pair<size_t, size_t>> edges;

            // Prim's algorithm for determining which POIs connect
            while (!unconnected.empty()) {
                float min_dist = 1e9f;
                size_t best_c = 0;
                size_t best_u_idx = 0;

                for (size_t c : connected) {
                    for (size_t u_idx = 0; u_idx < unconnected.size(); ++u_idx) {
                        size_t u = unconnected[u_idx];
                        float dx = static_cast<float>(region.pois[c].local_x - region.pois[u].local_x);
                        float dy = static_cast<float>(region.pois[c].local_y - region.pois[u].local_y);
                        float dist_val = std::sqrt(dx * dx + dy * dy);
                        
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

            // Route the determined edges cleanly using Dijkstra constrained to the local region
            for (const auto& edge : edges) {
                POI& start = region.pois[edge.first];
                POI& end = region.pois[edge.second];

                int start_x = start.local_x;
                int start_y = start.local_y;
                int end_x = end.local_x;
                int end_y = end.local_y;

                std::priority_queue<RoadNode, std::vector<RoadNode>, std::greater<RoadNode>> road_pq;
                std::vector<std::vector<float>> road_dist(30, std::vector<float>(70, 1e9f));
                std::vector<std::vector<Point>> parent(30, std::vector<Point>(70, {-1, -1}));

                road_pq.push({0.0f, start_x, start_y});
                road_dist[start_y][start_x] = 0.0f;

                int rdx[] = {0, 0, 1, -1, 1, 1, -1, -1};
                int rdy[] = {1, -1, 0, 0, 1, -1, 1, -1};

                while (!road_pq.empty()) {
                    RoadNode curr = road_pq.top();
                    road_pq.pop();

                    if (curr.x == end_x && curr.y == end_y) break;

                    for (int i = 0; i < 8; ++i) {
                        int nx = curr.x + rdx[i];
                        int ny = curr.y + rdy[i];

                        if (nx >= 0 && nx < 70 && ny >= 0 && ny < 30) {
                            LocalTile& ltile = region.local_grid[ny][nx];
                            
                            // Absolutely forbid routing outside the organic region mask
                            if (!ltile.in_region) continue;

                            float move_cost = (i < 4) ? 1.0f : 1.414f;
                            
                            // Terrain penalties to encourage organic routing
                            if (ltile.biome == Biome::Swamp || ltile.symbol == "~") move_cost += 5.0f;
                            else if (ltile.biome == Biome::Mountains) move_cost += 4.0f;
                            else if (ltile.biome == Biome::Hills || ltile.biome == Biome::Forest) move_cost += 2.0f;

                            // Massively prefer routing over existing roads
                            if (ltile.is_road) move_cost *= 0.1f; 

                            if (curr.cost + move_cost < road_dist[ny][nx]) {
                                road_dist[ny][nx] = curr.cost + move_cost;
                                parent[ny][nx] = {curr.x, curr.y};
                                road_pq.push({road_dist[ny][nx], nx, ny});
                            }
                        }
                    }
                }

                // Reconstruct the path from end to start and paint it
                int cx = end_x;
                int cy = end_y;
                while (cx != -1 && cy != -1) {
                    if (!(cx == start_x && cy == start_y) && !(cx == end_x && cy == end_y)) {
                        LocalTile& ltile = region.local_grid[cy][cx];
                        ltile.symbol = "+";
                        ltile.color = Terminal::COLOR_YELLOW;
                        ltile.is_road = true;
                    }
                    Point p = parent[cy][cx];
                    cx = p.x;
                    cy = p.y;
                }

                start.connected_pois.push_back(end.id);
                end.connected_pois.push_back(start.id);
            }
        }
    }
}
