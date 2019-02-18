//
//   Copyright (c) 2011-2017 Luke Benstead https://simulant-engine.appspot.com
//
//     This file is part of Simulant.
//
//     Simulant is free software: you can redistribute it and/or modify
//     it under the terms of the GNU Lesser General Public License as published by
//     the Free Software Foundation, either version 3 of the License, or
//     (at your option) any later version.
//
//     Simulant is distributed in the hope that it will be useful,
//     but WITHOUT ANY WARRANTY; without even the implied warranty of
//     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//     GNU Lesser General Public License for more details.
//
//     You should have received a copy of the GNU Lesser General Public License
//     along with Simulant.  If not, see <http://www.gnu.org/licenses/>.
//

#include "rigid_body.h"
#include "simulation.h"
#include "../../nodes/actor.h"
#include "../../stage.h"
#include "../../deps/bounce/bounce.h"

// These are just to keep Bounce happy
bool b3PushProfileScope(char const*) { return false; }
void b3PopProfileScope() {}


namespace smlt {
namespace behaviours {


RigidBody::RigidBody(RigidBodySimulation* simulation):
    Body(simulation) {

}

RigidBody::~RigidBody() {

}

void RigidBody::set_linear_velocity(const Vec3& vel) {
    auto sim = simulation_.lock();
    if(!sim) {
        return;
    }

    b3Body* b = sim->bodies_.at(this);

    b3Vec3 v;
    to_b3vec3(vel, v);
    b->SetLinearVelocity(v);
}


void RigidBody::set_linear_damping(const float d) {
    auto sim = simulation_.lock();
    if(!sim) {
        return;
    }

    b3Body* b = sim->bodies_.at(this);
    b->SetLinearDamping(d);
}

void RigidBody::set_angular_damping(const float d) {
    auto sim = simulation_.lock();
    if(!sim) {
        return;
    }

    b3Body* b = sim->bodies_.at(this);
    b->SetAngularDamping(d);
}

void RigidBody::add_force(const Vec3 &force) {
    auto sim = simulation_.lock();
    if(!sim) {
        return;
    }

    b3Body* b = sim->bodies_.at(this);

    b3Vec3 v;
    to_b3vec3(force, v);
    b->ApplyForceToCenter(v, true);
}

void RigidBody::add_relative_force(const Vec3 &force) {
    auto sim = simulation_.lock();
    if(!sim) {
        return;
    }

    b3Body* b = sim->bodies_.at(this);

    b3Vec3 v;
    to_b3vec3(force, v);

    // same as above, but convert the passed force vector to world space
    b->ApplyForceToCenter(b->GetWorldVector(v), true);
}

void RigidBody::add_relative_torque(const Vec3 &torque) {
    auto sim = simulation_.lock();
    if(!sim) {
        return;
    }

    b3Body* b = sim->bodies_.at(this);
    b3Vec3 t;
    to_b3vec3(torque, t);

    // Convert the vector to world space then apply
    b->ApplyTorque(b->GetWorldVector(t), true);
}

void RigidBody::add_impulse(const Vec3& impulse) {
    auto sim = simulation_.lock();
    if(!sim) {
        return;
    }

    b3Body* b = sim->bodies_.at(this);

    b3Vec3 v;
    to_b3vec3(impulse, v);
    b->ApplyLinearImpulse(v, b->GetPosition(), true);
}

void RigidBody::add_impulse_at_position(const Vec3& impulse, const Vec3& position) {
    auto sim = simulation_.lock();
    if(!sim) {
        return;
    }

    b3Body* b = sim->bodies_.at(this);

    b3Vec3 i, p;
    to_b3vec3(impulse, i);
    to_b3vec3(position, p);
    b->ApplyLinearImpulse(i, p, true);
}

float RigidBody::mass() const {
    auto sim = simulation_.lock();
    if(!sim) {
        return 0;
    }

    const b3Body* b = sim->bodies_.at(this);
    return b->GetMass();
}

Vec3 RigidBody::linear_velocity() const {
    auto sim = simulation_.lock();
    if(!sim) {
        return Vec3();
    }

    const b3Body* b = sim->bodies_.at(this);

    Vec3 v;
    to_vec3(b->GetLinearVelocity(), v);
    return v;
}

Vec3 RigidBody::angular_velocity() const {
    auto sim = simulation_.lock();
    if(!sim) {
        return Vec3();
    }

    const b3Body* b = sim->bodies_.at(this);

    Vec3 v;
    to_vec3(b->GetAngularVelocity(), v);
    return v;
}

Vec3 RigidBody::linear_velocity_at(const Vec3& position) const {
    auto sim = simulation_.lock();
    if(!sim) {
        return Vec3();
    }

    const b3Body* b = sim->bodies_.at(this);

    b3Vec3 bv;
    to_b3vec3(position, bv);
    auto direction_to_point = bv - b->GetPosition();
    auto relative_torque = b3Cross(b->GetAngularVelocity(), direction_to_point);

    Vec3 v;
    to_vec3(b->GetLinearVelocity() + relative_torque, v);
    return v;
}

Vec3 RigidBody::position() const {
    auto sim = simulation_.lock();
    if(!sim) {
        return Vec3();
    }

    return sim->body_transform(this).first;
}

void RigidBody::set_center_of_mass(const smlt::Vec3& com) {
    auto sim = simulation_.lock();
    if(!sim) {
        return;
    }

    b3Body* b = sim->bodies_.at(this);

    b3MassData data;
    b->GetMassData(&data);
    to_b3vec3(com, data.center);
    b->SetMassData(&data);
}

Quaternion RigidBody::rotation() const {
    auto sim = simulation_.lock();
    if(!sim) {
        return Quaternion();
    }

    return sim->body_transform(this).second;
}

void RigidBody::add_force_at_position(const Vec3& force, const Vec3& position) {
    auto sim = simulation_.lock();
    if(!sim) {
        return;
    }

    b3Body* b = sim->bodies_.at(this);

    b3Vec3 f, p;
    to_b3vec3(force, f);
    to_b3vec3(position, p);

    b->ApplyForce(f, p, true);
}

void RigidBody::add_torque(const Vec3& torque) {
    auto sim = simulation_.lock();
    if(!sim) {
        return;
    }

    b3Body* b = sim->bodies_.at(this);
    b3Vec3 t;
    to_b3vec3(torque, t);
    b->ApplyTorque(t, true);
}

bool RigidBody::is_awake() const {
    auto sim = simulation_.lock();
    if(!sim) {
        return false;
    }

    b3Body* b = sim->bodies_.at(this);
    return b->IsAwake();
}

}
}
