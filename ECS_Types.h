#ifndef ECS_TYPES_H
#define ECS_TYPES_H

#include <cstdint>
#include <bitset>

// Type aliases for the ECS
using Entity = std::uint32_t;
using ComponentType = std::uint8_t;

// Global ECS Constants
const Entity MAX_ENTITIES = 5000;
const ComponentType MAX_COMPONENTS = 32;

// Signature represents which components an entity has currently attached
using Signature = std::bitset<MAX_COMPONENTS>;

#endif // ECS_TYPES_H
