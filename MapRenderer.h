#ifndef MAPRENDERER_H
#define MAPRENDERER_H

#include "MapData.h"

class MapRenderer {
public:
    // Renders the 70x30 local_grid of the target region natively.
    // Masks out tiles that belong to other regions with a dark gray '#' border effect.
    // Draws the player '@' at the provided micro-coordinates (local_x, local_y).
    static void RenderRegion(const WorldMap& map, int target_region_id, int player_x = -1, int player_y = -1);

    // Renders a 70x30 viewport of the macroscopic continental map via subsampling.
    // Highlights the kingdom where the player is currently located with a blink effect.
    static void RenderContinentalMap(const WorldMap& map, int player_kingdom_id, int player_x = -1, int player_y = -1);

    // Renders the macro-grid 1:1 using the kingdom's min_x and min_y as the origin.
    // Colors regions individually and highlights the player's location using macro-coordinates.
    static void RenderKingdomMap(const WorldMap& map, int target_kingdom_id, int player_x = -1, int player_y = -1);
};

#endif // MAPRENDERER_H
