#include "EntityManager.h"
#include <stdexcept>

EntityManager::EntityManager() : mLivingEntityCount(0) {
    // Initialize the queue with all possible entity IDs
    for (Entity entity = 0; entity < MAX_ENTITIES; ++entity) {
        mAvailableEntities.push(entity);
    }
}

Entity EntityManager::CreateEntity() {
    if (mAvailableEntities.empty()) {
        throw std::runtime_error("Maximum number of entities reached.");
    }

    // Take an ID from the front of the queue
    Entity id = mAvailableEntities.front();
    mAvailableEntities.pop();
    ++mLivingEntityCount;

    return id;
}

void EntityManager::DestroyEntity(Entity e) {
    if (e >= MAX_ENTITIES) {
        throw std::out_of_range("Entity ID out of range.");
    }

    // Invalidate the destroyed entity's signature
    mSignatures[e].reset();

    // Put the destroyed ID at the back of the queue
    mAvailableEntities.push(e);
    --mLivingEntityCount;
}

void EntityManager::SetSignature(Entity e, Signature s) {
    if (e >= MAX_ENTITIES) {
        throw std::out_of_range("Entity ID out of range.");
    }

    // Update the entity's signature
    mSignatures[e] = s;
}

Signature EntityManager::GetSignature(Entity e) {
    if (e >= MAX_ENTITIES) {
        throw std::out_of_range("Entity ID out of range.");
    }

    // Retrieve the entity's signature
    return mSignatures[e];
}
