#ifndef MAPDATA_H
#define MAPDATA_H

#include <vector>
#include <unordered_map>
#include <string>

// Enum representing the different environmental biomes
enum class Biome {
    Plains,
    Forest,
    Hills,
    Mountains,
    Swamp,
    Desert
};

// Represents a single cell on the Continental map grid
struct Tile {
    Biome biome;
    int kingdom_id;
    int region_id;
    std::string symbol = " ";
    std::string color;
    bool is_road = false;
};

// Represents a single cell on the high-res Regional map grid
struct LocalTile {
    Biome biome;
    std::string symbol = " ";
    std::string color;
    bool is_road = false;
    bool in_region = false;
};

// Point of Interest (Settlements, Dungeons, Quest Locations)
struct POI {
    int id;
    int macro_x; // Coordinate on the 200x200 Continental Map
    int macro_y; // Coordinate on the 200x200 Continental Map
    int local_x; // Coordinate on the 70x30 Regional Map
    int local_y; // Coordinate on the 70x30 Regional Map
    std::string symbol;
    std::vector<int> connected_pois;
};

// A sub-section of a Kingdom containing multiple POIs and its own high-res map
struct Region {
    int id;
    int kingdom_id;
    std::vector<POI> pois;
    
    // 70x30 local grid initialized (30 rows, 70 columns)
    std::vector<std::vector<LocalTile>> local_grid = std::vector<std::vector<LocalTile>>(30, std::vector<LocalTile>(70));
};

// A major political entity made up of multiple Regions
struct Kingdom {
    int id;
    Biome primary_biome;
    std::vector<int> region_ids;
    
    // Bounding box for the Kingdom on the macro map
    int min_x = 200;
    int max_x = -1;
    int min_y = 200;
    int max_y = -1;
};

// The primary container for the macroscopic game world
class WorldMap {
public:
    // Initializes the 200x200 grid upon construction
    WorldMap() : grid(200, std::vector<Tile>(200)) {}

    // Generates the organic continental map using a Voronoi diagram approach
    void GenerateWorld(unsigned int seed = 1337);

    // 2D grid containing the overarching Continental map tiles
    std::vector<std::vector<Tile>> grid;

    // Fast lookup for Kingdoms and Regions by their IDs
    std::unordered_map<int, Kingdom> kingdoms;
    std::unordered_map<int, Region> regions;
};

#endif // MAPDATA_H
