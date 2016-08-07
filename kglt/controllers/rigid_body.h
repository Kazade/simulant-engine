#pragma once

#include <queue>

#include "controller.h"

#include "../generic/managed.h"
#include "../types.h"

#include "yocto/yocto_symrigid.h"


namespace kglt {
namespace controllers {


class RigidBodySimulation:
    public Managed<RigidBodySimulation> {

public:
    RigidBodySimulation(uint32_t max_bodies=1000);
    bool init() override;
    void cleanup() override;

    void step(double dt);

private:
    friend class RigidBody;

    ysr_scene* scene_ = nullptr;
    uint32_t max_bodies_ = 0;


    /* Yocto is designed to have a fixed number of bodies throughout the simulation
     * so what we do is create a max number, and disable them all by giving them all a zero
     * mass. We maintain a list of indexes to bodies which are "free" (as in, disabled) and
     * pop an index each time we create a body, and push it again when we free the body */
    std::queue<uint32_t> free_bodies_;


    // Used by the RigidBodyController on creation/destruction to register a body
    // in the simulation
    uint32_t acquire_body();
    void release_body(uint32_t i);
};

/*
 * A rigid body controller that uses the rather excellent Yocto-gl library
 * for rigid body simulation
 */
class RigidBody:
    public Controller {

public:
    RigidBody(Controllable* object, RigidBodySimulation::ptr simulation);
    ~RigidBody();

    void add_force(const Vec3& force);
    void add_force_at_position(const Vec3& force, const Vec3& position);
    void add_relative_force(const Vec3& force);

    void add_torque(const Vec3& torque);
    void add_relative_torque(const Vec3& torque);

    void move_to(const Vec3& position);
    void rotate_to(const Quaternion& rotation);

private:
    uint32_t body_id_ = 0;
    RigidBodySimulation::ptr simulation_;
};

}
}
