#include "dynamic_physics_body.h"
#include "../services/physics.h"

namespace smlt {

Vec3 DynamicPhysicsBody::forward() {
    Quaternion rot = simulated_rotation();
    return Vec3::NEGATIVE_Z * rot;
}

Vec3 DynamicPhysicsBody::right() {
    Quaternion rot = simulated_rotation();
    return Vec3::POSITIVE_X * rot;
}

Vec3 DynamicPhysicsBody::up() {
    Quaternion rot = simulated_rotation();
    return Vec3::POSITIVE_Y * rot;
}

Vec3 DynamicPhysicsBody::simulated_position() const {
    auto sim = get_simulation();
    assert(sim);
    return sim->body_position(this);
}

Quaternion DynamicPhysicsBody::simulated_rotation() const {
    auto sim = get_simulation();
    assert(sim);
    return sim->body_rotation(this);
}

}
