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

// Colliders available for auto generation on RigidBody construction
enum GeneratedColliderType {
    GENERATED_COLLIDER_TYPE_NONE,
    GENERATED_COLLIDER_TYPE_BOX,
    GENERATED_COLLIDER_TYPE_SPHERE,
    GENERATED_COLLIDER_TYPE_MESH
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
};


}
}
