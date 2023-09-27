#include "physics_body.h"
#include "bounce/bounce.h"
#include "../services/physics.h"
#include "stage_node.h"
#include "../scenes/scene.h"

namespace smlt {

// FIXME: Calculate these values from real-world values!
const PhysicsMaterial PhysicsMaterial::WOOD(0.005, 0.4, 0.2);
const PhysicsMaterial PhysicsMaterial::RUBBER(0.001, 0.3, 0.8);
const PhysicsMaterial PhysicsMaterial::IRON(0.1, 0.2, 0.00001);
const PhysicsMaterial PhysicsMaterial::STONE(0.1, 0.8, 0.00001);

const static auto& w = PhysicsMaterial::WOOD;
const static auto& r = PhysicsMaterial::RUBBER;
const static auto& i = PhysicsMaterial::IRON;
const static auto& s = PhysicsMaterial::STONE;

const PhysicsMaterial PhysicsMaterial::WOOD_25(w.density * 0.25f, w.friction, w.bounciness);
const PhysicsMaterial PhysicsMaterial::WOOD_50(w.density * 0.50f, w.friction, w.bounciness);
const PhysicsMaterial PhysicsMaterial::WOOD_75(w.density * 0.75f, w.friction, w.bounciness);

const PhysicsMaterial PhysicsMaterial::RUBBER_25(r.density * 0.25f, r.friction, r.bounciness);
const PhysicsMaterial PhysicsMaterial::RUBBER_50(r.density * 0.50f, r.friction, r.bounciness);
const PhysicsMaterial PhysicsMaterial::RUBBER_75(r.density * 0.75f, r.friction, r.bounciness);

const PhysicsMaterial PhysicsMaterial::IRON_25(i.density * 0.25f, i.friction, i.bounciness);
const PhysicsMaterial PhysicsMaterial::IRON_50(i.density * 0.50f, i.friction, i.bounciness);
const PhysicsMaterial PhysicsMaterial::IRON_75(i.density * 0.75f, i.friction, i.bounciness);

const PhysicsMaterial PhysicsMaterial::STONE_25(s.density * 0.25f, s.friction, s.bounciness);
const PhysicsMaterial PhysicsMaterial::STONE_50(s.density * 0.50f, s.friction, s.bounciness);
const PhysicsMaterial PhysicsMaterial::STONE_75(s.density * 0.75f, s.friction, s.bounciness);

PhysicsBody::PhysicsBody(StageNode *self, PhysicsBodyType type):
    self_(self),
    type_(type) {

    auto sim = get_simulation();
    if(sim) {
        sim->register_body(this);
    } else {
        S_WARN("PhysicsBody added without an active PhysicsService");
    }
}

PhysicsBody::~PhysicsBody() {
    auto sim = get_simulation();
    if(sim) {
        sim->unregister_body(this);
    }
}

void PhysicsBody::add_box_collider(
    const Vec3 &size, const PhysicsMaterial &properties, uint16_t kind, const Vec3 &offset, const Quaternion &rotation
    ) {
    auto simulation = get_simulation();
    if(!simulation) {
        return;
    }

    simulation->add_box_collider(this, size, properties, kind, offset, rotation);
}

void PhysicsBody::add_sphere_collider(const float diameter, const PhysicsMaterial& properties, uint16_t kind, const Vec3& offset) {
    auto simulation = get_simulation();
    if(!simulation) {
        return;
    }

    simulation->add_sphere_collider(this, diameter, properties, kind, offset);
}

void PhysicsBody::add_triangle_collider(
    const smlt::Vec3& v1, const smlt::Vec3& v2, const smlt::Vec3& v3,
    const PhysicsMaterial& properties, uint16_t kind
    ) {

    auto simulation = get_simulation();
    if(!simulation) {
        return;
    }

    simulation->add_triangle_collider(this, v1, v2, v3, properties, kind);
}

void PhysicsBody::add_capsule_collider(float height, const float diameter, const PhysicsMaterial& properties, uint16_t kind) {
    auto simulation = get_simulation();
    if(!simulation) {
        return;
    }

    return simulation->add_capsule_collider(this, height, diameter, properties, kind);
}

void PhysicsBody::register_collision_listener(CollisionListener *listener) {
    listeners_.insert(listener);
    listener->watching_.insert(this);
}

void PhysicsBody::unregister_collision_listener(CollisionListener *listener) {
    listener->watching_.erase(this);
    listeners_.erase(listener);
}

void PhysicsBody::contact_started(const Collision &collision) {
    for(auto listener: listeners_) {
        listener->on_collision_enter(collision);
    }
}

void PhysicsBody::contact_finished(const Collision& collision) {
    for(auto listener: listeners_) {
        listener->on_collision_exit(collision);
    }
}

PhysicsService* PhysicsBody::get_simulation() const {
    /* Caches the PhysicsService for perf */
    if(!simulation_) {
        /* FIXME: If the physics service is destroyed, we need
             * to wipe this out for every physics body */
        simulation_ = self_->scene->find_service<PhysicsService>();
    }

    return simulation_;
}

}
