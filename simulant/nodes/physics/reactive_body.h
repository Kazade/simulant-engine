#pragma once

#include "physics_body.h"

namespace smlt {

class SphereJoint;

class ReactiveBody : public PhysicsBody {
public:
    ReactiveBody(Scene* owner, StageNodeType node_type, PhysicsBodyType type):
        PhysicsBody(owner, node_type, type) {}

    SphereJoint* create_sphere_joint(
        ReactiveBody* other,
        const smlt::Vec3& this_relative_anchor,
        const smlt::Vec3& other_relative_anchor
    );

    std::size_t sphere_joint_count() const {
        return sphere_joints_.size();
    }

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
    void add_acceleration_force(const Vec3& acc) {
        add_force(acc * mass());
    }

    void add_acceleration_force_at_position(const Vec3& force, const Vec3& position) {
        add_force_at_position(force * mass(), position);
    }

    void set_linear_damping(const float d);

    void set_angular_damping(const float d) {
        set_angular_damping(Vec3(d, d, d));
    }

    void set_angular_damping(const Vec3& d);
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

    bool on_destroy() override;

private:
    std::vector<std::shared_ptr<SphereJoint>> sphere_joints_;
};

}
