#include "dynamic_physics_body.h"
#include "../services/physics.h"
#include "bounce/bounce.h"

namespace smlt {

float DynamicPhysicsBody::mass() const {
    auto sim = get_simulation();
    b3Body* b = (b3Body*) sim->private_body(this);
    return b->GetMass();
}

void DynamicPhysicsBody::set_mass(float m) {
    auto sim = get_simulation();
    b3Body* b = (b3Body*) sim->private_body(this);
    b3MassData data;
    b->GetMassData(&data);

    data.mass = m;
    b->SetMassData(&data);
}

void DynamicPhysicsBody::set_center_of_mass(const Vec3 &com) {
    auto sim = get_simulation();
    b3Body* b = (b3Body*) sim->private_body(this);
    b3MassData md;
    b3Vec3 new_center(com.x, com.y, com.z);

    b->GetMassData(&md);

    // Shift to old center of mass
    b3Mat33 Ic = md.I - md.mass * b3Steiner(md.center);

    // Compute new inertia (Shift inertia to local body origin)
    b3Mat33 I2 = Ic + md.mass * b3Steiner(new_center);

    md.center = new_center;
    md.I = I2;

    b->SetMassData(&md);
}

Vec3 DynamicPhysicsBody::center_of_mass() const {
    auto sim = get_simulation();
    b3Body* b = (b3Body*) sim->private_body(this);
    b3MassData data;
    b->GetMassData(&data);

    smlt::Vec3 ret(data.center.x, data.center.y, data.center.z);
    return ret;
}

Vec3 DynamicPhysicsBody::absolute_center_of_mass() const {
    auto sim = get_simulation();
    auto b = (b3Body*) sim->private_body(this);
    b3Vec3 center = b->GetWorldCenter();
    smlt::Vec3 ret(center.x, center.y, center.z);
    return ret;
}

void DynamicPhysicsBody::add_force(const Vec3 &force) {
    auto sim = get_simulation();
    auto b = (b3Body*) sim->private_body(this);

    b3Vec3 v(force.x, force.y, force.z);
    b->ApplyForceToCenter(v, true);
}

void DynamicPhysicsBody::add_force_at_position(const Vec3 &force, const Vec3 &position) {
    auto sim = get_simulation();
    auto b = (b3Body*) sim->private_body(this);
    b3Vec3 f(force.x, force.y, force.z);
    b3Vec3 p(position.x, position.y, position.z);
    b->ApplyForce(f, p, true);
}

void DynamicPhysicsBody::add_relative_force(const Vec3 &force) {
    auto sim = get_simulation();
    auto b = (b3Body*) sim->private_body(this);

    b3Vec3 v(force.x, force.y, force.z);
    // same as above, but convert the passed force vector to world space
    b->ApplyForceToCenter(b->GetWorldVector(v), true);
}

void DynamicPhysicsBody::add_torque(const Vec3 &torque) {
    auto sim = get_simulation();
    auto b = (b3Body*) sim->private_body(this);

    b3Vec3 t(torque.x, torque.y, torque.z);
    b->ApplyTorque(t, true);
}

void DynamicPhysicsBody::add_relative_torque(const Vec3 &torque) {
    auto sim = get_simulation();
    auto b = (b3Body*) sim->private_body(this);

    b3Vec3 t(torque.x, torque.y, torque.z);
    // Convert the vector to world space then apply
    b->ApplyTorque(b->GetWorldVector(t), true);
}

void DynamicPhysicsBody::add_impulse(const Vec3 &impulse) {
    auto sim = get_simulation();
    auto b = (b3Body*) sim->private_body(this);
    b3Vec3 v(impulse.x, impulse.y, impulse.z);
    b->ApplyLinearImpulse(v, b->GetPosition(), true);
}

void DynamicPhysicsBody::add_impulse_at_position(const Vec3 &impulse, const Vec3 &position) {
    auto sim = get_simulation();
    auto b = (b3Body*) sim->private_body(this);
    b3Vec3 i(impulse.x, impulse.y, impulse.z);
    b3Vec3 p(position.x, position.y, position.z);
    b->ApplyLinearImpulse(i, p, true);
}

void DynamicPhysicsBody::set_linear_damping(const float d) {
    auto sim = get_simulation();
    auto b = (b3Body*) sim->private_body(this);

    b3Vec3 v(d, d, d);
    b->SetLinearDamping(v);
}

void DynamicPhysicsBody::set_angular_damping(const Vec3 &d) {
    auto sim = get_simulation();
    auto b = (b3Body*) sim->private_body(this);

    b3Vec3 v(d.x, d.y, d.z);
    b->SetAngularDamping(v);
}

void DynamicPhysicsBody::set_angular_sleep_tolerance(float x) {
    auto sim = get_simulation();
    auto b = (b3Body*) sim->private_body(this);
    b->SetAngularSleepTolerance(x);
}

Vec3 DynamicPhysicsBody::linear_velocity() const {
    auto sim = get_simulation();
    auto b = (b3Body*) sim->private_body(this);

    auto v = b->GetLinearVelocity();
    return Vec3(v.x, v.y, v.z);
}

Vec3 DynamicPhysicsBody::linear_velocity_at(const Vec3 &position) const {
    auto sim = get_simulation();
    auto b = (b3Body*) sim->private_body(this);

    b3Vec3 bv(position.x, position.y, position.z);
    auto direction_to_point = bv - b->GetPosition();
    auto relative_torque = b3Cross(b->GetAngularVelocity(), direction_to_point);

    auto v = b->GetLinearVelocity() + relative_torque;
    return Vec3(v.x, v.y, v.z);
}

void DynamicPhysicsBody::set_linear_velocity(const Vec3 &vel) {
    auto sim = get_simulation();
    auto b = (b3Body*) sim->private_body(this);
    b3Vec3 v(vel.x, vel.y, vel.z);
    b->SetLinearVelocity(v);
}

Vec3 DynamicPhysicsBody::angular_velocity() const {
    auto sim = get_simulation();
    auto b = (b3Body*) sim->private_body(this);
    auto v = b->GetAngularVelocity();
    return Vec3(v.x, v.y, v.z);
}

void DynamicPhysicsBody::set_angular_velocity(const Vec3 &vel) {
    auto sim = get_simulation();
    auto b = (b3Body*) sim->private_body(this);
    b3Vec3 v(vel.x, vel.y, vel.z);
    b->SetAngularVelocity(v);
}

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

bool DynamicPhysicsBody::is_awake() const {
    auto sim = get_simulation();
    auto b = (b3Body*) sim->private_body(this);
    return b->IsAwake();
}

void DynamicPhysicsBody::lock_rotation(bool x, bool y, bool z) {
    auto sim = get_simulation();
    auto b = (b3Body*) sim->private_body(this);
    b->SetFixedRotation(x, y, z);
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
