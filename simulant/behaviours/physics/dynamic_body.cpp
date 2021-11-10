#include "bounce/bounce.h"

#include "simulation.h"
#include "dynamic_body.h"

namespace smlt {
namespace behaviours {
namespace impl {

b3Body* DynamicBody::fetch_body() const {
    auto sim = simulation_.lock();
    if(!sim) {
        return nullptr;
    }

    b3Body* b = sim->bodies_.at(this);
    assert(b);
    return b;
}



void DynamicBody::lock_rotation(bool x, bool y, bool z) {
    auto sim = simulation_.lock();
    if(!sim) {
        return;
    }

    b3Body* b = sim->bodies_.at(this);
    return b->SetFixedRotation(x, y, z);
}

void DynamicBody::set_linear_velocity(const Vec3& vel) {
    auto sim = simulation_.lock();
    if(!sim) {
        return;
    }

    b3Body* b = sim->bodies_.at(this);

    b3Vec3 v;
    to_b3vec3(vel, v);
    b->SetLinearVelocity(v);
}

void DynamicBody::set_angular_velocity(const Vec3& vel) {
    auto sim = simulation_.lock();
    if(!sim) {
        return;
    }

    b3Body* b = sim->bodies_.at(this);

    b3Vec3 v;
    to_b3vec3(vel, v);
    b->SetAngularVelocity(v);
}

void DynamicBody::set_linear_damping(const float d) {
    auto sim = simulation_.lock();
    if(!sim) {
        return;
    }

    b3Body* b = sim->bodies_.at(this);

    b3Vec3 v;
    v.x = d;
    v.y = d;
    v.z = d;

    b->SetLinearDamping(v);
}

void DynamicBody::set_angular_damping(const float d) {
    set_angular_damping(Vec3(d, d, d));
}

void DynamicBody::set_angular_damping(const Vec3& vec) {
    auto sim = simulation_.lock();
    if(!sim) {
        return;
    }

    b3Body* b = sim->bodies_.at(this);

    b3Vec3 v;
    to_b3vec3(vec, v);
    b->SetAngularDamping(v);
}

void DynamicBody::set_angular_sleep_tolerance(float x) {
    auto sim = simulation_.lock();
    if(!sim) {
        return;
    }

    b3Body* b = sim->bodies_.at(this);
    b->SetAngularSleepTolerance(x);
}

void DynamicBody::add_force(const Vec3 &force) {
    auto sim = simulation_.lock();
    if(!sim) {
        return;
    }

    b3Body* b = sim->bodies_.at(this);

    b3Vec3 v;
    to_b3vec3(force, v);
    b->ApplyForceToCenter(v, true);
}

void DynamicBody::add_relative_force(const Vec3 &force) {
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

void DynamicBody::add_relative_torque(const Vec3 &torque) {
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

void DynamicBody::add_impulse(const Vec3& impulse) {
    auto sim = simulation_.lock();
    if(!sim) {
        return;
    }

    b3Body* b = sim->bodies_.at(this);

    b3Vec3 v;
    to_b3vec3(impulse, v);
    b->ApplyLinearImpulse(v, b->GetPosition(), true);
}

void DynamicBody::add_impulse_at_position(const Vec3& impulse, const Vec3& position) {
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

void DynamicBody::add_acceleration_force(const Vec3 &acc) {
    add_force(acc * mass());
}

void DynamicBody::add_acceleration_force_at_position(const Vec3 &force, const Vec3 &position) {
    add_force_at_position(force * mass(), position);
}

Vec3 DynamicBody::linear_velocity() const {
    auto sim = simulation_.lock();
    if(!sim) {
        return Vec3();
    }

    const b3Body* b = sim->bodies_.at(this);

    Vec3 v;
    to_vec3(b->GetLinearVelocity(), v);
    return v;
}

Vec3 DynamicBody::angular_velocity() const {
    auto sim = simulation_.lock();
    if(!sim) {
        return Vec3();
    }

    const b3Body* b = sim->bodies_.at(this);

    Vec3 v;
    to_vec3(b->GetAngularVelocity(), v);
    return v;
}

Vec3 DynamicBody::linear_velocity_at(const Vec3& position) const {
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

Vec3 DynamicBody::position() const {
    auto sim = simulation_.lock();
    if(!sim) {
        return Vec3();
    }

    return sim->body_transform(this).first;
}

Quaternion DynamicBody::rotation() const {
    auto sim = simulation_.lock();
    if(!sim) {
        return Quaternion();
    }

    return sim->body_transform(this).second;
}

void DynamicBody::add_force_at_position(const Vec3& force, const Vec3& position) {
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

void DynamicBody::add_torque(const Vec3& torque) {
    auto sim = simulation_.lock();
    if(!sim) {
        return;
    }

    b3Body* b = sim->bodies_.at(this);
    b3Vec3 t;
    to_b3vec3(torque, t);
    b->ApplyTorque(t, true);
}

bool DynamicBody::is_awake() const {
    auto sim = simulation_.lock();
    if(!sim) {
        return false;
    }

    b3Body* b = sim->bodies_.at(this);
    return b->IsAwake();
}

void DynamicBody::set_center_of_mass(const smlt::Vec3& com) {
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

void DynamicBody::set_mass(float m) {
    auto b = fetch_body();

    b3MassData data;
    b->GetMassData(&data);

    data.mass = m;
    b->SetMassData(&data);
}

float DynamicBody::mass() const {
    auto b = fetch_body();
    return b->GetMass();
}


}
}
}
