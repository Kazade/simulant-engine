#include "bounce/bounce.h"

#include "simulation.h"
#include "dynamic_body.h"

namespace smlt {
namespace behaviours {
namespace impl {

static inline void to_b3vec3(const Vec3& rhs, b3Vec3& ret) {
    ret.x = rhs.x;
    ret.y = rhs.y;
    ret.z = rhs.z;
}

static inline void to_vec3(const b3Vec3& rhs, Vec3& ret) {
    ret.x = rhs.x;
    ret.y = rhs.y;
    ret.z = rhs.z;
}

b3Body* DynamicBody::fetch_body() const {
    assert(simulation_);

    assert(body_);
    return body_;
}


void DynamicBody::lock_rotation(bool x, bool y, bool z) {
    assert(simulation_);

    b3Body* b = fetch_body();
    return b->SetFixedRotation(x, y, z);
}

void DynamicBody::set_linear_velocity(const Vec3& vel) {
    assert(simulation_);

    b3Body* b = fetch_body();

    b3Vec3 v;
    to_b3vec3(vel, v);
    b->SetLinearVelocity(v);
}

void DynamicBody::set_angular_velocity(const Vec3& vel) {
    assert(simulation_);

    b3Body* b = fetch_body();

    b3Vec3 v;
    to_b3vec3(vel, v);
    b->SetAngularVelocity(v);
}

void DynamicBody::set_linear_damping(const float d) {
    assert(simulation_);

    b3Body* b = fetch_body();

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
    assert(simulation_);

    b3Body* b = fetch_body();

    b3Vec3 v;
    to_b3vec3(vec, v);
    b->SetAngularDamping(v);
}

void DynamicBody::set_angular_sleep_tolerance(float x) {
    assert(simulation_);

    b3Body* b = fetch_body();
    b->SetAngularSleepTolerance(x);
}

void DynamicBody::add_force(const Vec3 &force) {
    assert(simulation_);

    b3Body* b = fetch_body();

    b3Vec3 v;
    to_b3vec3(force, v);
    b->ApplyForceToCenter(v, true);
}

void DynamicBody::add_relative_force(const Vec3 &force) {
    assert(simulation_);

    b3Body* b = fetch_body();

    b3Vec3 v;
    to_b3vec3(force, v);

    // same as above, but convert the passed force vector to world space
    b->ApplyForceToCenter(b->GetWorldVector(v), true);
}

void DynamicBody::add_relative_torque(const Vec3 &torque) {
    assert(simulation_);

    b3Body* b = fetch_body();
    b3Vec3 t;
    to_b3vec3(torque, t);

    // Convert the vector to world space then apply
    b->ApplyTorque(b->GetWorldVector(t), true);
}

void DynamicBody::add_impulse(const Vec3& impulse) {
    assert(simulation_);

    b3Vec3 v;
    to_b3vec3(impulse, v);
    body_->ApplyLinearImpulse(v, body_->GetPosition(), true);
}

void DynamicBody::add_impulse_at_position(const Vec3& impulse, const Vec3& position) {
    assert(simulation_);

    b3Vec3 i, p;
    to_b3vec3(impulse, i);
    to_b3vec3(position, p);
    body_->ApplyLinearImpulse(i, p, true);
}

void DynamicBody::add_acceleration_force(const Vec3 &acc) {
    add_force(acc * mass());
}

void DynamicBody::add_acceleration_force_at_position(const Vec3 &force, const Vec3 &position) {
    add_force_at_position(force * mass(), position);
}

Vec3 DynamicBody::linear_velocity() const {
    assert(simulation_);

    Vec3 v;
    to_vec3(body_->GetLinearVelocity(), v);
    return v;
}

Vec3 DynamicBody::angular_velocity() const {
    assert(simulation_);

    Vec3 v;
    to_vec3(body_->GetAngularVelocity(), v);
    return v;
}

Vec3 DynamicBody::linear_velocity_at(const Vec3& position) const {
    assert(simulation_);

    b3Vec3 bv;
    to_b3vec3(position, bv);
    auto direction_to_point = bv - body_->GetPosition();
    auto relative_torque = b3Cross(body_->GetAngularVelocity(), direction_to_point);

    Vec3 v;
    to_vec3(body_->GetLinearVelocity() + relative_torque, v);
    return v;
}

Vec3 DynamicBody::position() const {
    assert(simulation_);

    return simulation_->body_transform(this).first;
}

Quaternion DynamicBody::rotation() const {
    assert(simulation_);
    return simulation_->body_transform(this).second;
}

void DynamicBody::add_force_at_position(const Vec3& force, const Vec3& position) {
    assert(simulation_);

    b3Vec3 f, p;
    to_b3vec3(force, f);
    to_b3vec3(position, p);

    body_->ApplyForce(f, p, true);
}

void DynamicBody::add_torque(const Vec3& torque) {
    assert(simulation_);

    b3Vec3 t;
    to_b3vec3(torque, t);
    body_->ApplyTorque(t, true);
}

bool DynamicBody::is_awake() const {
    assert(simulation_);

    return body_->IsAwake();
}

void DynamicBody::set_center_of_mass(const smlt::Vec3& com) {
    assert(simulation_);

    b3MassData data;
    body_->GetMassData(&data);
    to_b3vec3(com, data.center);
    body_->SetMassData(&data);
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
