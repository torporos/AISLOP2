#include "Coordinator.h"
#include "Components.h"
#include "Input.h"
#include "Terminal.h"
#include "MapData.h"
#include "MapRenderer.h"
#include "GameState.h"
#include <algorithm>
#include <iostream>

// Instantiate the central ECS Coordinator
Coordinator gCoordinator;

// Helper function to find a POI by its global ID
POI GetPOIById(const WorldMap& world, int poi_id) {
    for (const auto& pair : world.regions) {
        for (const auto& poi : pair.second.pois) {
            if (poi.id == poi_id) return poi;
        }
    }
    return POI{}; // Fallback
}

int main() {
    // --- Terminal Setup ---
    Terminal::EnterAltScreen();
    Terminal::HideCursor();

    // 1. Initialize and Register Components
    gCoordinator.RegisterComponent<Position>();
    gCoordinator.RegisterComponent<Renderable>();
    gCoordinator.RegisterComponent<PlayerNavigation>();

    // 2. Set up Game State and World
    GameState currentState = GameState::RegionalMap;
    WorldMap world;
    world.GenerateWorld(1337);

    // 3. Create the Player Entity
    Entity player = gCoordinator.CreateEntity();

    // Spawn at the first POI of Region 0
    int start_kingdom_id = world.regions[0].kingdom_id;
    int start_region_id = world.regions[0].id;
    int start_poi_id = world.regions[0].pois[0].id;
    int start_x = world.regions[0].pois[0].x;
    int start_y = world.regions[0].pois[0].y;

    gCoordinator.AddComponent(player, Position{start_x, start_y});
    gCoordinator.AddComponent(player, Renderable{'@', Terminal::COLOR_GREEN});
    gCoordinator.AddComponent(player, PlayerNavigation{start_kingdom_id, start_region_id, start_poi_id});

    // 4. Main Game Loop
    bool isRunning = true;
    while (isRunning) {
        auto& nav = gCoordinator.GetComponent<PlayerNavigation>(player);
        auto& pos = gCoordinator.GetComponent<Position>(player);

        // Synchronize physical position with current POI logic
        POI current_poi = GetPOIById(world, nav.current_poi_id);
        pos.x = current_poi.x;
        pos.y = current_poi.y;
        nav.current_region_id = world.grid[pos.y][pos.x].region_id;
        nav.current_kingdom_id = world.grid[pos.y][pos.x].kingdom_id;

        // Render the current frame based on state
        if (currentState == GameState::RegionalMap) {
            MapRenderer::RenderRegion(world, nav.current_region_id, pos.x, pos.y);
        } else if (currentState == GameState::KingdomMap) {
            MapRenderer::RenderKingdomMap(world, nav.current_kingdom_id, nav.current_region_id);
        } else if (currentState == GameState::ContinentalMap) {
            MapRenderer::RenderContinentalMap(world, nav.current_kingdom_id, pos.x, pos.y);
        } else {
            // Placeholder: Clear the screen for Local maps
            Terminal::ClearScreen();
            Terminal::MoveCursor(2, 2);
            Terminal::SetColor(Terminal::COLOR_WHITE);
            if (currentState == GameState::LocalMap) std::cout << "Local Map View (Entering POI)";
            std::cout.flush();
        }

        // Block and wait for input
        char input = Input::GetCharWithoutEnter();

        // 5. Handle State Transitions & Graph-Based Input Navigation
        if (input == 'q') {
            isRunning = false;
        } else if (input == 'm') {
            // Zoom out
            if (currentState == GameState::RegionalMap) {
                currentState = GameState::KingdomMap;
            } else if (currentState == GameState::KingdomMap) {
                currentState = GameState::ContinentalMap;
            }
        } else if (input == 'i') {
            // Zoom in
            if (currentState == GameState::ContinentalMap) {
                currentState = GameState::KingdomMap;
            } else if (currentState == GameState::KingdomMap) {
                currentState = GameState::RegionalMap;
            }
        } else if (input == 'e' || input == '\n' || input == '\r') {
            // Enter POI
            if (currentState == GameState::RegionalMap) {
                currentState = GameState::LocalMap;
            }
        } else if (currentState == GameState::RegionalMap && (input == 'w' || input == 'a' || input == 's' || input == 'd')) {
            // Node-based Navigation Logic for Regional Map
            int best_poi_id = nav.current_poi_id;
            int max_diff = 0;

            for (int connected_id : current_poi.connected_pois) {
                POI connected_poi = GetPOIById(world, connected_id);

                int dx = connected_poi.x - current_poi.x;
                int dy = connected_poi.y - current_poi.y;

                if (input == 'd' && dx > 0 && dx > max_diff) {
                    best_poi_id = connected_id;
                    max_diff = dx;
                } else if (input == 'a' && dx < 0 && -dx > max_diff) {
                    best_poi_id = connected_id;
                    max_diff = -dx;
                } else if (input == 's' && dy > 0 && dy > max_diff) {
                    best_poi_id = connected_id;
                    max_diff = dy;
                } else if (input == 'w' && dy < 0 && -dy > max_diff) {
                    best_poi_id = connected_id;
                    max_diff = -dy;
                }
            }

            nav.current_poi_id = best_poi_id;
        }
    }

    // --- Terminal Teardown ---
    Terminal::ShowCursor();
    Terminal::ExitAltScreen();

    return 0;
}
