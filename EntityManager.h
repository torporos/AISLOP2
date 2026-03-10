#ifndef ENTITYMANAGER_H
#define ENTITYMANAGER_H

#include "ECS_Types.h"
#include <queue>
#include <array>

class EntityManager {
public:
    EntityManager();

    // Entity lifecycle
    Entity CreateEntity();
    void DestroyEntity(Entity e);

    // Signature management
    void SetSignature(Entity e, Signature s);
    Signature GetSignature(Entity e);

private:
    // Queue of unused entity IDs
    std::queue<Entity> mAvailableEntities;

    // Array of signatures where the index corresponds to the entity ID
    std::array<Signature, MAX_ENTITIES> mSignatures;

    // Optional: Keep track of how many entities are currently active
    std::uint32_t mLivingEntityCount;
};

#endif // ENTITYMANAGER_H
