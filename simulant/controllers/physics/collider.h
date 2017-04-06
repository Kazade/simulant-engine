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


}
}
