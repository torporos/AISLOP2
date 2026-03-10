#ifndef MAPRENDERER_H
#define MAPRENDERER_H

#include "MapData.h"

class MapRenderer {
public:
    // Renders a 70x30 viewport centered on the target region.
    // Masks out tiles that belong to other regions to create a "fog of war" border effect.
    // Optionally draws the player directly on the map coordinates if provided.
    static void RenderRegion(const WorldMap& map, int target_region_id, int player_x = -1, int player_y = -1);

    // Renders a 70x30 viewport of the macroscopic continental map via subsampling.
    // Highlights the kingdom where the player is currently located.
    static void RenderContinentalMap(const WorldMap& map, int player_kingdom_id, int player_x = -1, int player_y = -1);

    // Renders a 70x30 viewport scaled to fit the bounding box of a specific kingdom.
    // Colors regions individually and highlights the player's current region with an '@'.
    static void RenderKingdomMap(const WorldMap& map, int target_kingdom_id, int player_region_id);
};

#endif // MAPRENDERER_H
