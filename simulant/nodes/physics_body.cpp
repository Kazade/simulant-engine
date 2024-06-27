#include "physics_body.h"
#include "../application.h"
#include "../generic/raii.h"
#include "../scenes/scene.h"
#include "../services/physics.h"
#include "../time_keeper.h"
#include "bounce/bounce.h"
#include "simulant/math/quaternion.h"
#include "stage_node.h"

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

PhysicsBody::PhysicsBody(Scene *owner, StageNodeType node_type, PhysicsBodyType type):
    StageNode(owner, node_type),
    type_(type) {


}

PhysicsBody::~PhysicsBody() {
    assert(is_destroyed());
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

void PhysicsBody::add_capsule_collider(const Vec3& v0, const Vec3& v1, const float diameter, const PhysicsMaterial& properties, uint16_t kind) {
    auto simulation = get_simulation();
    if(!simulation) {
        return;
    }

    return simulation->add_capsule_collider(this, v0, v1, diameter, properties, kind);
}

void PhysicsBody::register_collision_listener(CollisionListener *listener) {
    listeners_.insert(listener);
    listener->watching_.insert(this);
}

void PhysicsBody::unregister_collision_listener(CollisionListener *listener) {
    listener->watching_.erase(this);
    listeners_.erase(listener);
}

Vec3 PhysicsBody::simulated_position() const {
    auto sim = get_simulation();
    if(!sim) {
        return {};
    }

    b3Body* b = (b3Body*) sim->private_body(this);
    if(b) {
        auto vec = b->GetTransform().translation;
        return Vec3(
            vec.x, vec.y, vec.z
        );
    }

    return {};
}

Quaternion PhysicsBody::simulated_rotation() const {
    auto sim = get_simulation();
    if(!sim) {
        return {};
    }

    b3Body* b = (b3Body*) sim->private_body(this);
    if(b) {
        auto vec = b->GetTransform().rotation;
        return Quaternion(
            vec.v.x, vec.v.y, vec.v.z, vec.s
        );
    }

    return {};
}

void PhysicsBody::set_simulated_position(const Vec3 &position) {
    auto sim = get_simulation();
    if(sim) {
        b3Body* b = (b3Body*) sim->private_body(this);
        if(b) {
            b3Vec3 p(position.x, position.y, position.z);
            b->SetTransform(p, b->GetTransform().rotation);
        }
    }
}

void PhysicsBody::set_simulated_rotation(const Quaternion &rotation) {
    auto sim = get_simulation();
    if(sim) {
        b3Body* b = (b3Body*) sim->private_body(this);
        if(b) {
            b3Quat q;
            q.v.x = rotation.x;
            q.v.y = rotation.y;
            q.v.z = rotation.z;
            q.s = rotation.w;
            b->SetTransform(b->GetPosition(), q.GetRotationMatrix());
        }
    }
}

void PhysicsBody::clear_simulation_cache() {
    simulation_ = nullptr;
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
        simulation_ = scene->find_service<PhysicsService>();
    }

    return simulation_;
}

bool PhysicsBody::on_create(ConstructionArgs* params) {
    auto initial_pos = params->arg<Vec3>("position").value_or(Vec3());
    auto initial_rot =
        params->arg<Quaternion>("orientation").value_or(Quaternion());
    auto sim = get_simulation();
    if(sim) {
        sim->register_body(this, initial_pos, initial_rot);
    } else {
        S_WARN("PhysicsBody added without an active PhysicsService");
    }

    /* FIXME: These should probably be create arguments for *all* stage nodes,
     * not just PhysicsBodies */
    transform->set_position(initial_pos);
    transform->set_orientation(initial_rot);

    return true;
}

bool PhysicsBody::on_destroy() {
    StageNode::on_destroy();

    auto sim = get_simulation();
    if(sim) {
        sim->unregister_body(this);
    }

    return true;
}

void PhysicsBody::on_transformation_changed() {
    StageNode::on_transformation_changed();

    // Ignore any signals that we've caused by us setting
    // the transform
    if(updating_body_) {
        return;
    }

    // If we're here, then the user called set_position or something
    // so we need to update the rigid body to where it was intended

    auto sim = get_simulation();
    b3Body* b = (b3Body*) sim->private_body(this);
    if(b) {
        auto o = transform->orientation();
        auto pos = transform->position();
        b3Vec3 p(pos.x, pos.y, pos.z);
        b3Quat r(o.x, o.y, o.z, o.w);

        b->SetTransform(p, r.GetRotationMatrix());
    }
}

void PhysicsBody::on_update(float dt) {
    updating_body_ = true;

    raii::Finally finally([=]() {
        updating_body_ = false;
    });

    if(transform->smoothing_mode() == TRANSFORM_SMOOTHING_EXTRAPOLATE) {
        auto prev_state = last_state_; // This is set by the signal connected in Body::Body()
        auto next_state = std::make_pair(
            simulated_position(),
            simulated_rotation()
        );

        // Prevent a divide by zero.
        float r = get_app()->time_keeper->fixed_step_remainder();
        float t = (dt == 0.0f) ? 0.0f : smlt::fast_divide(r, dt);

        auto new_pos = prev_state.first.lerp(next_state.first, t);
        auto new_rot = prev_state.second.slerp(next_state.second, t);

        transform->set_position(new_pos);
        transform->set_orientation(new_rot);
    } else {
        transform->set_position(simulated_position());
        transform->set_orientation(simulated_rotation());
    }
}

}
