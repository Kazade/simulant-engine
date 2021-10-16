#pragma once

#include "../../generic/managed.h"
#include "../../signals/signal.h"
#include "../../types.h"

#include "collider.h"


struct b3World;
struct b3Vec3;
struct b3Mat33;
struct b3Quat;
struct b3Body;

namespace smlt {

class TimeKeeper;

namespace behaviours {

namespace impl {
    class Body;
    class DynamicBody;
    class ContactListener;
}

typedef sig::signal<void ()> SimulationPreStepSignal;

class RigidBodySimulation:
    public RefCounted<RigidBodySimulation> {

    DEFINE_SIGNAL(SimulationPreStepSignal, signal_simulation_pre_step);

public:
    RigidBodySimulation(TimeKeeper* time_keeper);
    bool init() override;
    void clean_up() override;

    void fixed_update(float step);

    std::pair<Vec3, bool> intersect_ray(const Vec3& start, const Vec3& direction, float* distance=nullptr, Vec3 *normal=nullptr);

    void set_gravity(const Vec3& gravity);

    bool body_exists(const impl::Body* body) const { return bodies_.count(body); }
private:
    friend class impl::Body;
    friend class impl::DynamicBody;

    friend class RigidBody;
    friend class StaticBody;
    friend class KinematicBody;

    TimeKeeper* time_keeper_ = nullptr;

    std::shared_ptr<b3World> scene_;
    std::shared_ptr<impl::ContactListener> contact_listener_;

    // Used by the RigidBodyBehaviour on creation/destruction to register a body
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
void to_b3quat(const Quaternion& q, b3Quat& ret);

// FIXME: Rename the actual class
typedef RigidBodySimulation PhysicsSimulation;
typedef std::shared_ptr<RigidBodySimulation> PhysicsSimulationPtr;

}
}
