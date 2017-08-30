#pragma once

#include <vector>
#include "../../types.h"

namespace smlt {
namespace controllers {

// Colliders available to add_X_collider
enum ColliderType {
    /* OOB collisiion detection */
    COLLIDER_TYPE_BOX,
    COLLIDER_TYPE_SPHERE,
    COLLIDER_TYPE_MESH
};

struct PhysicsMaterial {
    PhysicsMaterial() = default;
    PhysicsMaterial(float density, float friction, float bounciness):
        density(density), friction(friction), bounciness(bounciness) {}

    float density = 0.0f;
    float friction = 0.0f;
    float bounciness = 0.0f;

    static const PhysicsMaterial WOOD;
    static const PhysicsMaterial RUBBER;
    static const PhysicsMaterial IRON;
    static const PhysicsMaterial STONE;

    static const PhysicsMaterial WOOD_25;
    static const PhysicsMaterial WOOD_50;
    static const PhysicsMaterial WOOD_75;

    static const PhysicsMaterial RUBBER_25;
    static const PhysicsMaterial RUBBER_50;
    static const PhysicsMaterial RUBBER_75;

    static const PhysicsMaterial IRON_25;
    static const PhysicsMaterial IRON_50;
    static const PhysicsMaterial IRON_75;

    static const PhysicsMaterial STONE_25;
    static const PhysicsMaterial STONE_50;
    static const PhysicsMaterial STONE_75;
};


}
}
