#pragma once

#include <queue>

#include "controller.h"

#include "../generic/managed.h"
#include "../types.h"

#include "yocto/yocto_symrigid.h"
#include "yocto/yocto_bvh.h"


namespace kglt {

class MoveableObject;

namespace controllers {

namespace impl {
    class Body;
}

class RigidBody;

class RigidBodySimulation:
    public Managed<RigidBodySimulation> {

public:
    RigidBodySimulation(uint32_t max_bodies=1000);
    bool init() override;
    void cleanup() override;

    void step(double dt);

    Vec3 intersect_ray(const Vec3& start, const Vec3& direction);

private:
    friend class impl::Body;
    friend class RigidBody;
    friend class StaticBody;

    ysr_scene* scene_ = nullptr;
    yb_scene* bvh_scene_ = nullptr;

    uint32_t max_bodies_ = 0;


    /* Yocto is designed to have a fixed number of bodies throughout the simulation
     * so what we do is create a max number, and disable them all by giving them all a zero
     * mass. We maintain a list of indexes to bodies which are "free" (as in, disabled) and
     * pop an index each time we create a body, and push it again when we free the body */
    std::queue<uint32_t> free_bodies_;


    // Used by the RigidBodyController on creation/destruction to register a body
    // in the simulation
    uint32_t acquire_body(impl::Body* body);
    void release_body(impl::Body *body);

    std::unordered_map<impl::Body*, uint32_t> bodies_;

    std::pair<Vec3, Quaternion> body_transform(impl::Body* body);
    void set_body_transform(impl::Body *body, const Vec3& position, const Quaternion& rotation);

    ysr__body* get_ysr_body(impl::Body* body);
};

enum ColliderType {
    COLLIDER_TYPE_NONE,
    COLLIDER_TYPE_MESH
};

namespace impl {
    class Body:
        public Controller {

    public:
        Body(Controllable* object, RigidBodySimulation::ptr simulation, ColliderType collider=COLLIDER_TYPE_MESH);
        virtual ~Body();

        void move_to(const Vec3& position);
        void rotate_to(const Quaternion& rotation);

    protected:
        friend class RigidBodySimulation;
        MoveableObject* object_;
        uint32_t body_id_ = 0;
        RigidBodySimulation::ptr simulation_;

        void do_post_update(double dt) override;

        void build_collider(ColliderType collider);

    private:
        struct Shape {
            std::vector<int> indexes;
            std::vector<ym_vec3f> vertices;
            int element_type = ysr_etype_triangle;
        } shape;
    };
} // End impl

/*
 * Almost the same as a rigid body, but has no mass, and doesn't take part in the simulation
 * aside from acting as a collider */
class StaticBody:
    public impl::Body,
    public Managed<StaticBody> {

public:
    StaticBody(Controllable* object, RigidBodySimulation::ptr simulation);
    ~StaticBody();
};

/*
 * A rigid body controller that uses the rather excellent Yocto-gl library
 * for rigid body simulation
 */
class RigidBody:
    public impl::Body,
    public Managed<RigidBody> {

public:
    RigidBody(Controllable* object, RigidBodySimulation::ptr simulation);
    ~RigidBody();

    void add_force(const Vec3& force);
    void add_force_at_position(const Vec3& force, const Vec3& position);
    void add_relative_force(const Vec3& force);

    void add_torque(const Vec3& torque);
    void add_relative_torque(const Vec3& torque);

private:
    friend class RigidBodySimulation;

    // Cleared each step
    Vec3 force_;
    Vec3 torque_;
};

}
}
