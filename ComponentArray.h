#ifndef COMPONENTARRAY_H
#define COMPONENTARRAY_H

#include "ECS_Types.h"
#include <array>
#include <unordered_map>
#include <stdexcept>

// Base class so we can store arrays of different types polymorphically
class IComponentArray {
public:
    virtual ~IComponentArray() = default;
    virtual void EntityDestroyed(Entity entity) = 0;
};

// Templated class to store components tightly packed in memory
template<typename T>
class ComponentArray : public IComponentArray {
public:
    ComponentArray() : mSize(0) {}

    // Adds a new component to the end of the array and updates maps
    void InsertData(Entity entity, T component) {
        if (mEntityToIndexMap.find(entity) != mEntityToIndexMap.end()) {
            throw std::runtime_error("Component added to same entity more than once.");
        }

        // Put new entry at end and update the maps
        size_t newIndex = mSize;
        mEntityToIndexMap[entity] = newIndex;
        mIndexToEntityMap[newIndex] = entity;
        mComponentArray[newIndex] = component;
        ++mSize;
    }

    // Removes a component and keeps the array packed by moving the last element
    void RemoveData(Entity entity) {
        if (mEntityToIndexMap.find(entity) == mEntityToIndexMap.end()) {
            throw std::runtime_error("Removing non-existent component.");
        }

        // Copy element at end into deleted element's place to maintain density
        size_t indexOfRemovedEntity = mEntityToIndexMap[entity];
        size_t indexOfLastElement = mSize - 1;
        mComponentArray[indexOfRemovedEntity] = mComponentArray[indexOfLastElement];

        // Update map to point to moved spot
        Entity entityOfLastElement = mIndexToEntityMap[indexOfLastElement];
        mEntityToIndexMap[entityOfLastElement] = indexOfRemovedEntity;
        mIndexToEntityMap[indexOfRemovedEntity] = entityOfLastElement;

        // Erase the old mappings
        mEntityToIndexMap.erase(entity);
        mIndexToEntityMap.erase(indexOfLastElement);

        --mSize;
    }

    // Retrieves a reference to an entity's component
    T& GetData(Entity entity) {
        if (mEntityToIndexMap.find(entity) == mEntityToIndexMap.end()) {
            throw std::runtime_error("Retrieving non-existent component.");
        }

        return mComponentArray[mEntityToIndexMap[entity]];
    }

    // Called when an entity is destroyed to ensure its components are cleaned up
    void EntityDestroyed(Entity entity) override {
        if (mEntityToIndexMap.find(entity) != mEntityToIndexMap.end()) {
            RemoveData(entity);
        }
    }

private:
    // The tightly packed array of components
    std::array<T, MAX_ENTITIES> mComponentArray;

    // Map from an entity ID to its array index
    std::unordered_map<Entity, size_t> mEntityToIndexMap;

    // Map from an array index to its corresponding entity ID
    std::unordered_map<size_t, Entity> mIndexToEntityMap;

    // Total number of valid entries in the array
    size_t mSize;
};

#endif // COMPONENTARRAY_H
