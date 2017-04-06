#pragma once

#include "../../generic/managed.h"
#include "../../deps/kazsignal/kazsignal.h"
#include "../../types.h"

#include "collider.h"


struct b3World;
struct b3Vec3;
struct b3Mat33;
struct b3Quat;
struct b3Body;

namespace smlt {

class TimeKeeper;

namespace controllers {

namespace impl {
    class Body;
}

typedef sig::signal<void ()> SimulationPreStepSignal;

class RigidBodySimulation:
    public Managed<RigidBodySimulation>,
    public std::enable_shared_from_this<RigidBodySimulation> {

    DEFINE_SIGNAL(SimulationPreStepSignal, signal_simulation_pre_step);

public:
    RigidBodySimulation(TimeKeeper* time_keeper);
    bool init() override;
    void cleanup() override;

    void fixed_update(float step);

    std::pair<Vec3, bool> intersect_ray(const Vec3& start, const Vec3& direction, float* distance=nullptr, Vec3 *normal=nullptr);

    void set_gravity(const Vec3& gravity);

private:
    friend class impl::Body;
    friend class RigidBody;
    friend class StaticBody;

    TimeKeeper* time_keeper_ = nullptr;

    std::shared_ptr<b3World> scene_;

    // Used by the RigidBodyController on creation/destruction to register a body
    // in the simulation
    b3Body *acquire_body(impl::Body* body);
    void release_body(impl::Body *body);

    std::unordered_map<const impl::Body*, b3Body*> bodies_;

    std::pair<Vec3, Quaternion> body_transform(const impl::Body *body);
    void set_body_transform(impl::Body *body, const Vec3& position, const Quaternion& rotation);
};

void to_b3vec3(const Vec3& rhs, b3Vec3& ret);
void to_vec3(const b3Vec3& rhs, Vec3& ret);
void to_mat3(const b3Mat33& rhs, Mat3& out);
void to_quat(const b3Quat& rhs, Quaternion& out);

}
}
