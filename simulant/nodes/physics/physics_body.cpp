#include "physics_body.h"
#include "../../application.h"
#include "../../generic/raii.h"
#include "../../scenes/scene.h"
#include "../../services/physics.h"
#include "../../time_keeper.h"
#include "../stage_node.h"
#include "bounce/bounce.h"
#include "private.h"
#include "simulant/math/quaternion.h"
#include <memory>

namespace smlt {

PhysicsBody::PhysicsBody(Scene* owner, StageNodeType node_type,
                         PhysicsBodyType type) :
    StageNode(owner, node_type), type_(type) {}

PhysicsBody::~PhysicsBody() {
    assert(is_destroyed());
}

void PhysicsBody::add_box_collider(const Vec3& size,
                                   const PhysicsMaterial& properties,
                                   uint16_t kind, const Vec3& offset,
                                   const Quaternion& rotation) {
    auto simulation = get_simulation();
    if(!simulation) {
        return;
    }

    simulation->add_box_collider(this, size, properties, kind, offset,
                                 rotation);
}

void PhysicsBody::add_sphere_collider(const float diameter,
                                      const PhysicsMaterial& properties,
                                      uint16_t kind, const Vec3& offset) {
    auto simulation = get_simulation();
    if(!simulation) {
        return;
    }

    simulation->add_sphere_collider(this, diameter, properties, kind, offset);
}

void PhysicsBody::add_triangle_collider(const smlt::Vec3& v1,
                                        const smlt::Vec3& v2,
                                        const smlt::Vec3& v3,
                                        const PhysicsMaterial& properties,
                                        uint16_t kind) {

    auto simulation = get_simulation();
    if(!simulation) {
        return;
    }

    simulation->add_triangle_collider(this, v1, v2, v3, properties, kind);
}

void PhysicsBody::add_capsule_collider(const Vec3& v0, const Vec3& v1,
                                       const float diameter,
                                       const PhysicsMaterial& properties,
                                       uint16_t kind) {
    auto simulation = get_simulation();
    if(!simulation) {
        return;
    }

    return simulation->add_capsule_collider(this, v0, v1, diameter, properties,
                                            kind);
}

void PhysicsBody::register_collision_listener(CollisionListener* listener) {
    listeners_.insert(listener);
    listener->watching_.insert(this);
}

void PhysicsBody::unregister_collision_listener(CollisionListener* listener) {
    listener->watching_.erase(this);
    listeners_.erase(listener);
}

Vec3 PhysicsBody::position() const {
    if(!bounce_) {
        return {};
    }

    b3Body* b = bounce_->body;
    if(b) {
        auto vec = b->GetTransform().translation;
        return Vec3(vec.x, vec.y, vec.z);
    }

    return {};
}

Quaternion PhysicsBody::orientation() const {
    if(!bounce_) {
        return {};
    }

    b3Body* b = bounce_->body;
    if(b) {
        auto vec = b->GetTransform().rotation;
        return Quaternion(vec.v.x, vec.v.y, vec.v.z, vec.s);
    }

    return {};
}

void PhysicsBody::set_position(const Vec3& position) {
    if(bounce_) {
        b3Body* b = bounce_->body;
        if(b) {
            b3Vec3 p(position.x, position.y, position.z);
            b->SetTransform(p, b->GetTransform().rotation);
        }
    }
}

void PhysicsBody::set_orientation(const Quaternion& rotation) {
    if(bounce_) {
        b3Body* b = bounce_->body;
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

void PhysicsBody::contact_started(const Collision& collision) {
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
    return scene->find_service<PhysicsService>();
}

bool PhysicsBody::on_create(Params params) {
    if(!clean_params<PhysicsBody>(params)) {
        return false;
    }

    bool ret = StageNode::on_create(params);
    if(!ret) {
        return false;
    }

    Vec3 initial_pos = transform->position();
    Quaternion initial_rot = transform->orientation();

    auto sim = get_simulation();
    if(sim) {
        assert(!bounce_);
        bounce_ = std::make_unique<_impl::BounceData>();

        sim->register_body(this, initial_pos, initial_rot);
    } else {
        S_ERROR("PhysicsBody added without an active PhysicsService");
    }

    return true;
}

bool PhysicsBody::on_destroy() {
    StageNode::on_destroy();

    auto sim = get_simulation();
    if(sim) {
        sim->unregister_body(this);
    }

    bounce_.reset();

    return true;
}

void PhysicsBody::on_transformation_changed() {
    StageNode::on_transformation_changed();

    // Ignore any signals that we've caused by us setting
    // the transform
    if(updating_body_ || !bounce_) {
        return;
    }

    // If we're here, then the user called set_position or something
    // so we need to update the rigid body to where it was intended

    b3Body* b = bounce_->body;
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
        auto prev_state =
            last_state_; // This is set by the signal connected in Body::Body()
        auto next_state =
            std::make_pair(position(), orientation());

        // Prevent a divide by zero.
        float r = get_app()->time_keeper->fixed_step_remainder();
        float t = (dt == 0.0f) ? 0.0f : smlt::fast_divide(r, dt);

        auto new_pos = prev_state.first.lerp(next_state.first, t);
        auto new_rot = prev_state.second.slerp(next_state.second, t);

        transform->set_position(new_pos);
        transform->set_orientation(new_rot);
    } else {
        transform->set_position(position());
        transform->set_orientation(orientation());
    }
}

} // namespace smlt
