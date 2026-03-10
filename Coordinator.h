#ifndef COORDINATOR_H
#define COORDINATOR_H

#include "ECS_Types.h"
#include "EntityManager.h"
#include "ComponentArray.h"
#include <memory>
#include <unordered_map>
#include <typeindex>
#include <typeinfo>
#include <stdexcept>

class Coordinator {
public:
    Coordinator() : mNextComponentType(0) {
        mEntityManager = std::make_unique<EntityManager>();
    }

    // Entity methods
    Entity CreateEntity() {
        return mEntityManager->CreateEntity();
    }

    void DestroyEntity(Entity entity) {
        mEntityManager->DestroyEntity(entity);

        // Notify all component arrays that an entity has been destroyed
        // so they can remove the entity's components and maintain packed arrays
        for (auto const& pair : mComponentArrays) {
            auto const& componentArray = pair.second;
            componentArray->EntityDestroyed(entity);
        }
    }

    // Component methods
    template<typename T>
    void RegisterComponent() {
        std::type_index typeName = typeid(T);

        if (mComponentTypes.find(typeName) != mComponentTypes.end()) {
            throw std::runtime_error("Registering component type more than once.");
        }

        // Add this component type to the component type map
        mComponentTypes[typeName] = mNextComponentType;

        // Create a ComponentArray pointer and add it to the component arrays map
        mComponentArrays[typeName] = std::make_shared<ComponentArray<T>>();

        ++mNextComponentType;
    }

    template<typename T>
    void AddComponent(Entity entity, T component) {
        // Add the component to the array
        GetComponentArray<T>()->InsertData(entity, component);

        // Update the entity's signature
        auto signature = mEntityManager->GetSignature(entity);
        signature.set(mComponentTypes[typeid(T)], true);
        mEntityManager->SetSignature(entity, signature);
    }

    template<typename T>
    void RemoveComponent(Entity entity) {
        // Remove the component from the array
        GetComponentArray<T>()->RemoveData(entity);

        // Update the entity's signature
        auto signature = mEntityManager->GetSignature(entity);
        signature.set(mComponentTypes[typeid(T)], false);
        mEntityManager->SetSignature(entity, signature);
    }

    template<typename T>
    T& GetComponent(Entity entity) {
        return GetComponentArray<T>()->GetData(entity);
    }

private:
    std::unique_ptr<EntityManager> mEntityManager;

    // Map from type string pointer to a component type
    std::unordered_map<std::type_index, ComponentType> mComponentTypes;

    // Map from type string pointer to a component array
    std::unordered_map<std::type_index, std::shared_ptr<IComponentArray>> mComponentArrays;

    // The component type to be assigned to the next registered component
    ComponentType mNextComponentType;

    // Convenience function to get the statically casted pointer to the ComponentArray of type T
    template<typename T>
    std::shared_ptr<ComponentArray<T>> GetComponentArray() {
        std::type_index typeName = typeid(T);

        if (mComponentTypes.find(typeName) == mComponentTypes.end()) {
            throw std::runtime_error("Component not registered before use.");
        }

        return std::static_pointer_cast<ComponentArray<T>>(mComponentArrays[typeName]);
    }
};

#endif // COORDINATOR_H
