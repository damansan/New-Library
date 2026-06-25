#pragma once
#include <cstdint>

enum class CollisionLayer : uint32_t
{
    None      = 0,
    Static    = 1 << 0,
    Dynamic   = 1 << 1,
    Kinematic = 1 << 2,
    Character = 1 << 3,
};

enum class PhysicsAttribute : uint32_t
{
    None = 0,
    Walkable = 1 << 0,
};

inline uint32_t operator|(CollisionLayer a, CollisionLayer b)
{
    return static_cast<uint32_t>(a) | static_cast<uint32_t>(b);
}

inline uint32_t operator|(PhysicsAttribute a, PhysicsAttribute b)
{
    return static_cast<uint32_t>(a) | static_cast<uint32_t>(b);
}