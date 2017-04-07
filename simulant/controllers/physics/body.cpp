
#include "body.h"
#include "simulation.h"

#include "../../nodes/stage_node.h"
#include "../../nodes/actor.h"
#include "../../time_keeper.h"
#include "../../deps/bounce/bounce.h"
#include "../../stage.h"

namespace smlt {
namespace controllers {
namespace impl {

Body::Body(Controllable* object, RigidBodySimulation* simulation, GeneratedColliderType collider_type):
    Controller(),
    simulation_(simulation->shared_from_this()),
    collider_type_(collider_type) {

    object_ = dynamic_cast<StageNode*>(object);
    if(!object_) {
        throw std::runtime_error("Tried to attach a rigid body controller to something that isn't moveable");
    }

    // Keep each body's last_state in sync when the simulation is stepped
    simulation_stepped_connection_ = simulation->signal_simulation_pre_step().connect([this]() {
        if(auto sim = simulation_.lock()) {
            last_state_ = sim->body_transform(this);
        }
    });
}

Body::~Body() {
    simulation_stepped_connection_.disconnect();
}

bool Body::init() {
    auto sim = simulation_.lock();
    if(!sim) {
        return false;
    }

    body_ = sim->acquire_body(this);
    build_collider(collider_type_);

    return true;
}

void Body::cleanup() {
    auto sim = simulation_.lock();
    if(sim) {
        sim->release_body(this);
    }
}

void Body::move_to(const Vec3& position) {
    auto sim = simulation_.lock();
    if(!sim) {
        return;
    }

    auto xform = sim->body_transform(this);
    sim->set_body_transform(
        this,
        position,
        xform.second
    );

    object_->move_to_absolute(position);
}

void Body::update(float dt) {
    const bool INTERPOLATION_ENABLED = true;

    auto sim = simulation_.lock();
    if(!sim) {
        return;
    }

    if(INTERPOLATION_ENABLED) {
        auto prev_state = last_state_; // This is set by the signal connected in Body::Body()
        auto next_state = sim->body_transform(this);

        float t = sim->time_keeper_->fixed_step_remainder() / dt;

        auto new_pos = prev_state.first.lerp(next_state.first, t);
        auto new_rot = prev_state.second.slerp(next_state.second, t);

        object_->move_to_absolute(new_pos);
        object_->rotate_to_absolute(new_rot);
    } else {
        auto state = sim->body_transform(this);
        object_->move_to_absolute(state.first);
        object_->rotate_to_absolute(state.second);
    }
}

void Body::build_collider(GeneratedColliderType collider) {
    if(collider == GENERATED_COLLIDER_TYPE_NONE) {
        return;
    }

    auto sim = simulation_.lock();
    if(!sim) {
        return;
    }

    if(collider == GENERATED_COLLIDER_TYPE_BOX) {
        BoundableEntity* entity = dynamic_cast<BoundableEntity*>(object_);
        if(entity) {
            AABB aabb = entity->aabb();

            auto def = std::make_shared<b3BoxHull>();
            def->Set(aabb.width() * 0.5, aabb.height() * 0.5, aabb.depth() * 0.5);
            hulls_.push_back(def);

            b3HullShape hsdef;
            hsdef.m_hull = def.get();

            b3ShapeDef sdef;
            sdef.shape = &hsdef;
            sdef.userData = this;
            sdef.density = 0.005;
            sdef.friction = 0.3;

            sim->bodies_.at(this)->CreateShape(sdef);
        }
    } else {
        assert(0 && "Not Implemented");
    }
}

}
}

}
