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

// Point of Interest (Settlements, Dungeons, Quest Locations)
struct POI {
    int id;
    int x;
    int y;
    char symbol;
    std::vector<int> connected_pois;
};

// A sub-section of a Kingdom containing multiple POIs
struct Region {
    int id;
    int kingdom_id;
    std::vector<POI> pois;
};

// A major political entity made up of multiple Regions
struct Kingdom {
    int id;
    Biome primary_biome;
    std::vector<int> region_ids;
};

// Represents a single cell on the Continental map grid
struct Tile {
    Biome biome;
    int kingdom_id;
    int region_id;
    char symbol = ' ';
    std::string color;
    bool is_road = false;
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
