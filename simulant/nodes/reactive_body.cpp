#include "reactive_body.h"
#include "../services/physics.h"
#include "bounce/bounce.h"
#include "joints.h"

namespace smlt {

SphereJoint *ReactiveBody::create_sphere_joint(
    ReactiveBody *other, const Vec3 &this_relative_anchor, const Vec3 &other_relative_anchor) {

    sphere_joints_.push_back(
        std::make_shared<SphereJoint>(
            this, other, this_relative_anchor, other_relative_anchor
        )
    );

    // Add the joint to the lists on both objects
    other->sphere_joints_.push_back(sphere_joints_.back());

    return sphere_joints_.back().get();
}

float ReactiveBody::mass() const {
    auto sim = get_simulation();
    if(!sim) {
        return 0.0f;
    }

    b3Body* b = (b3Body*) sim->private_body(this);
    return b->GetMass();
}

void ReactiveBody::set_mass(float m) {
    auto sim = get_simulation();
    if(!sim) {
        return;
    }

    b3Body* b = (b3Body*) sim->private_body(this);
    b3MassData data;
    b->GetMassData(&data);

    data.mass = m;
    b->SetMassData(&data);
}

void ReactiveBody::set_center_of_mass(const Vec3 &com) {
    auto sim = get_simulation();
    if(!sim) {
        return;
    }

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

Vec3 ReactiveBody::center_of_mass() const {
    auto sim = get_simulation();
    if(!sim) {
        return Vec3();
    }

    b3Body* b = (b3Body*) sim->private_body(this);
    b3MassData data;
    b->GetMassData(&data);

    smlt::Vec3 ret(data.center.x, data.center.y, data.center.z);
    return ret;
}

Vec3 ReactiveBody::absolute_center_of_mass() const {
    auto sim = get_simulation();
    if(!sim) {
        return Vec3();
    }

    auto b = (b3Body*) sim->private_body(this);
    b3Vec3 center = b->GetWorldCenter();
    smlt::Vec3 ret(center.x, center.y, center.z);
    return ret;
}

void ReactiveBody::add_force(const Vec3 &force) {
    auto sim = get_simulation();
    if(!sim) {
        return;
    }

    auto b = (b3Body*) sim->private_body(this);

    b3Vec3 v(force.x, force.y, force.z);
    b->ApplyForceToCenter(v, true);
}

void ReactiveBody::add_force_at_position(const Vec3 &force, const Vec3 &position) {
    auto sim = get_simulation();
    if(!sim) {
        return;
    }

    auto b = (b3Body*) sim->private_body(this);
    b3Vec3 f(force.x, force.y, force.z);
    b3Vec3 p(position.x, position.y, position.z);
    b->ApplyForce(f, p, true);
}

void ReactiveBody::add_relative_force(const Vec3 &force) {
    auto sim = get_simulation();
    if(!sim) {
        return;
    }

    auto b = (b3Body*) sim->private_body(this);

    b3Vec3 v(force.x, force.y, force.z);
    // same as above, but convert the passed force vector to world space
    b->ApplyForceToCenter(b->GetWorldVector(v), true);
}

void ReactiveBody::add_torque(const Vec3 &torque) {
    auto sim = get_simulation();
    if(!sim) {
        return;
    }

    auto b = (b3Body*) sim->private_body(this);

    b3Vec3 t(torque.x, torque.y, torque.z);
    b->ApplyTorque(t, true);
}

void ReactiveBody::add_relative_torque(const Vec3 &torque) {
    auto sim = get_simulation();
    if(!sim) {
        return;
    }

    auto b = (b3Body*) sim->private_body(this);

    b3Vec3 t(torque.x, torque.y, torque.z);
    // Convert the vector to world space then apply
    b->ApplyTorque(b->GetWorldVector(t), true);
}

void ReactiveBody::add_impulse(const Vec3 &impulse) {
    auto sim = get_simulation();
    if(!sim) {
        return;
    }

    auto b = (b3Body*) sim->private_body(this);
    b3Vec3 v(impulse.x, impulse.y, impulse.z);
    b->ApplyLinearImpulse(v, b->GetPosition(), true);
}

void ReactiveBody::add_impulse_at_position(const Vec3 &impulse, const Vec3 &position) {
    auto sim = get_simulation();
    if(!sim) {
        return;
    }

    auto b = (b3Body*) sim->private_body(this);
    b3Vec3 i(impulse.x, impulse.y, impulse.z);
    b3Vec3 p(position.x, position.y, position.z);
    b->ApplyLinearImpulse(i, p, true);
}

void ReactiveBody::set_linear_damping(const float d) {
    auto sim = get_simulation();
    if(!sim) {
        return;
    }

    auto b = (b3Body*) sim->private_body(this);

    b3Vec3 v(d, d, d);
    b->SetLinearDamping(v);
}

void ReactiveBody::set_angular_damping(const Vec3 &d) {
    auto sim = get_simulation();
    if(!sim) {
        return;
    }

    auto b = (b3Body*) sim->private_body(this);

    b3Vec3 v(d.x, d.y, d.z);
    b->SetAngularDamping(v);
}

void ReactiveBody::set_angular_sleep_tolerance(float x) {
    auto sim = get_simulation();
    if(!sim) {
        return;
    }

    auto b = (b3Body*) sim->private_body(this);
    b->SetAngularSleepTolerance(x);
}

Vec3 ReactiveBody::linear_velocity() const {
    auto sim = get_simulation();
    if(!sim) {
        return Vec3();
    }

    auto b = (b3Body*) sim->private_body(this);

    auto v = b->GetLinearVelocity();
    return Vec3(v.x, v.y, v.z);
}

Vec3 ReactiveBody::linear_velocity_at(const Vec3 &position) const {
    auto sim = get_simulation();
    if(!sim) {
        return {};
    }

    auto b = (b3Body*) sim->private_body(this);

    b3Vec3 bv(position.x, position.y, position.z);
    auto direction_to_point = bv - b->GetPosition();
    auto relative_torque = b3Cross(b->GetAngularVelocity(), direction_to_point);

    auto v = b->GetLinearVelocity() + relative_torque;
    return Vec3(v.x, v.y, v.z);
}

void ReactiveBody::set_linear_velocity(const Vec3 &vel) {
    auto sim = get_simulation();
    if(!sim) {
        return;
    }

    auto b = (b3Body*) sim->private_body(this);
    b3Vec3 v(vel.x, vel.y, vel.z);
    b->SetLinearVelocity(v);
}

Vec3 ReactiveBody::angular_velocity() const {
    auto sim = get_simulation();
    if(!sim) {
        return Vec3();
    }

    auto b = (b3Body*) sim->private_body(this);
    auto v = b->GetAngularVelocity();
    return Vec3(v.x, v.y, v.z);
}

void ReactiveBody::set_angular_velocity(const Vec3 &vel) {
    auto sim = get_simulation();
    if(!sim) {
        return;
    }

    auto b = (b3Body*) sim->private_body(this);
    b3Vec3 v(vel.x, vel.y, vel.z);
    b->SetAngularVelocity(v);
}

Vec3 ReactiveBody::forward() {
    Quaternion rot = simulated_rotation();
    return Vec3::forward() * rot;
}

Vec3 ReactiveBody::right() {
    Quaternion rot = simulated_rotation();
    return Vec3::right() * rot;
}

Vec3 ReactiveBody::up() {
    Quaternion rot = simulated_rotation();
    return Vec3::up() * rot;
}

bool ReactiveBody::is_awake() const {
    auto sim = get_simulation();
    if(!sim) {
        return false;
    }

    auto b = (b3Body*) sim->private_body(this);
    return b->IsAwake();
}

void ReactiveBody::lock_rotation(bool x, bool y, bool z) {
    auto sim = get_simulation();
    if(!sim) {
        return;
    }

    auto b = (b3Body*) sim->private_body(this);
    b->SetFixedRotation(x, y, z);
}

bool ReactiveBody::on_destroy() {

    // Remove the sphere joints from the counterpart body
    for(auto joint: sphere_joints_) {
        auto& other = joint->second_body()->sphere_joints_;
        other.erase(std::remove(other.begin(), other.end(), joint), other.end());
    }

    // Clear the sphere joints from this node, this will destroy
    // the joints and remove them from the simulation
    sphere_joints_.clear();

    return PhysicsBody::on_destroy();
}


}
