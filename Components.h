#ifndef COMPONENTS_H
#define COMPONENTS_H

#include <string>

// Pure data structure for an entity's 2D map position
struct Position {
    int x;
    int y;
};

// Pure data structure for visual representation in the terminal
struct Renderable {
    char symbol;
    std::string color;
};

// Data structure tracking graph-based navigation status and map hierarchy
struct PlayerNavigation {
    int current_kingdom_id;
    int current_region_id;
    int current_poi_id;
};

#endif // COMPONENTS_H
