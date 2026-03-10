#include "MapData.h"
#include "Terminal.h"
#include <random>
#include <cmath>
#include <vector>
#include <queue>

// Helper function for bilinear interpolation
float BilinearInterpolate(float tx, float ty, float c00, float c10, float c01, float c11) {
    float a = c00 * (1.0f - tx) + c10 * tx;
    float b = c01 * (1.0f - tx) + c11 * tx;
    return a * (1.0f - ty) + b * ty;
}

// Data structure for Dijkstra expansion
struct ExpandNode {
    float cost;
    int x;
    int y;
    int kingdom_id;

    // We want a min-heap, so greater cost means lower priority
    bool operator>(const ExpandNode& other) const {
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

    // Clear previous regions and kingdoms since this is a pre-kingdom terrain generation pass
    kingdoms.clear();
    regions.clear();

    // 2. Upscale to 200x200 using bilinear interpolation and assign Biomes
    for (int y = 0; y < 200; ++y) {
        for (int x = 0; x < 200; ++x) {
            // Map 200x200 coordinates to the 20x20 grid (scale factor = 10)
            float gx = x / 10.0f;
            float gy = y / 10.0f;

            int x0 = static_cast<int>(gx);
            int y0 = static_cast<int>(gy);
            int x1 = x0 + 1;
            int y1 = y0 + 1;

            float tx = gx - x0;
            float ty = gy - y0;

            // Interpolate Elevation
            float e00 = lowResElev[y0][x0];
            float e10 = lowResElev[y0][x1];
            float e01 = lowResElev[y1][x0];
            float e11 = lowResElev[y1][x1];
            float elev = BilinearInterpolate(tx, ty, e00, e10, e01, e11);

            // Interpolate Moisture
            float m00 = lowResMoist[y0][x0];
            float m10 = lowResMoist[y0][x1];
            float m01 = lowResMoist[y1][x0];
            float m11 = lowResMoist[y1][x1];
            float moist = BilinearInterpolate(tx, ty, m00, m10, m01, m11);

            // Initialize the tile
            Tile& tile = grid[y][x];
            tile.kingdom_id = -1; // -1 denotes unclaimed land
            tile.region_id = -1;
            tile.is_road = false;

            // 3. Assign Biome based on intersection of Elevation and Moisture
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
        Biome::Plains,     // 0: Human
        Biome::Forest,     // 1: Elf
        Biome::Hills,      // 2: Gnome
        Biome::Mountains,  // 3: Dwarf
        Biome::Swamp,      // 4: Troll
        Biome::Desert      // 5: Lizardman
    };

    for (int i = 0; i < 6; ++i) {
        // Initialize Kingdom data
        Kingdom k;
        k.id = i;
        k.primary_biome = preferredBiomes[i];
        kingdoms[i] = k;

        // Find a valid starting tile matching the preferred biome
        int sx, sy;
        while (true) {
            sx = coord_dist(gen);
            sy = coord_dist(gen);

            // Ensure no two kingdoms spawn on the same tile and matches biome
            if (grid[sy][sx].biome == preferredBiomes[i] && grid[sy][sx].kingdom_id == -1) {
                break;
            }
        }

        // Seed the priority queue
        pq.push({0.0f, sx, sy, i});
        min_cost[sy][sx] = 0.0f;
    }

    // 5. Multi-source Dijkstra Expansion
    int dx[] = {0, 0, 1, -1};
    int dy[] = {1, -1, 0, 0};

    while (!pq.empty()) {
        ExpandNode current = pq.top();
        pq.pop();

        // If this tile has already been claimed, skip
        if (grid[current.y][current.x].kingdom_id != -1) {
            continue;
        }

        // Claim the tile
        grid[current.y][current.x].kingdom_id = current.kingdom_id;

        // Expand to neighbors
        for (int i = 0; i < 4; ++i) {
            int nx = current.x + dx[i];
            int ny = current.y + dy[i];

            // Check bounds and if tile is unclaimed
            if (nx >= 0 && nx < 200 && ny >= 0 && ny < 200) {
                if (grid[ny][nx].kingdom_id == -1) {
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
}
