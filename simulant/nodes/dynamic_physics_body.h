#pragma once

#include "physics_body.h"


namespace smlt {

class DynamicPhysicsBody : public PhysicsBody {
public:
    DynamicPhysicsBody(StageNode* self, PhysicsBodyType type):
        PhysicsBody(self, type) {}

    float mass() const;
    void set_mass(float m);
    void set_center_of_mass(const smlt::Vec3& com);
    Vec3 center_of_mass() const;
    Vec3 absolute_center_of_mass() const;

    void add_force(const Vec3& force);
    void add_force_at_position(const Vec3& force, const Vec3& position);
    void add_relative_force(const Vec3& force);

    void add_torque(const Vec3& torque);
    void add_relative_torque(const Vec3& torque);

    void add_impulse(const Vec3& impulse);
    void add_impulse_at_position(const Vec3& impulse, const Vec3& position);

    /** Add a force to the body over time, ignoring mass */
    void add_acceleration_force(const Vec3& acc);
    void add_acceleration_force_at_position(const Vec3& force, const Vec3& position);

    void set_linear_damping(const float d);
    void set_angular_damping(const float d);
    void set_angular_damping(const Vec3& v);

    void set_angular_sleep_tolerance(float x);

    Vec3 linear_velocity() const;
    Vec3 linear_velocity_at(const Vec3& position) const;
    void set_linear_velocity(const Vec3& vel);

    Vec3 angular_velocity() const;
    void set_angular_velocity(const Vec3& vel);

    Vec3 forward();
    Vec3 right();
    Vec3 up();

    bool is_awake() const;

    void lock_rotation(bool x, bool y, bool z);

    Vec3 simulated_position() const;
    Quaternion simulated_rotation() const;
};

}
