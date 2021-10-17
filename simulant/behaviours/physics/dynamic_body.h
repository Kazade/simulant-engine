#pragma once

#include "body.h"

namespace smlt {
namespace behaviours {
namespace impl {

class DynamicBody:
    public impl::Body {

public:
    DynamicBody(RigidBodySimulation* simulation):
        Body(simulation) {}

    void add_force(const Vec3& force);
    void add_force_at_position(const Vec3& force, const Vec3& position);
    void add_relative_force(const Vec3& force);

    void add_torque(const Vec3& torque);
    void add_relative_torque(const Vec3& torque);

    void add_impulse(const Vec3& impulse);
    void add_impulse_at_position(const Vec3& impulse, const Vec3& position);

    void set_linear_damping(const float d);
    void set_angular_damping(const float d);
    void set_angular_damping(const Vec3& v);

    void set_angular_sleep_tolerance(float x);

    Vec3 linear_velocity() const;
    Vec3 linear_velocity_at(const Vec3& position) const;
    void set_linear_velocity(const Vec3& vel);

    Vec3 angular_velocity() const;

    Vec3 position() const;
    Quaternion rotation() const;

    Vec3 forward() {
        Quaternion rot = rotation();
        return Vec3::NEGATIVE_Z * rot;
    }

    Vec3 right() {
        Quaternion rot = rotation();
        return Vec3::POSITIVE_X * rot;
    }

    Vec3 up() {
        Quaternion rot = rotation();
        return Vec3::POSITIVE_Y * rot;
    }

    bool is_awake() const;
};


}
}
}
