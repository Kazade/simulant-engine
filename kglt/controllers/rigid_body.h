#pragma once

#include <queue>
#include <kazmath/ray3.h>

#include "controller.h"

#include "../generic/managed.h"
#include "../types.h"

#include "qu3e/q3.h"

namespace kglt {

class MoveableObject;

namespace controllers {

namespace impl {
    class Body;
}


struct Triangle {
    uint32_t index[3];
    Vec3 normal;
};


struct RaycastCollider {
    std::vector<Vec3> vertices;
    std::vector<Triangle> triangles;
    Vec3 normal;

    std::pair<Vec3, bool> intersect_ray(const Vec3& start, const Vec3& direction, float* hit_distance=nullptr) {
        kmRay3 ray;
        kmVec3Assign(&ray.start, &start);
        kmVec3Assign(&ray.dir, &direction);

        float closest_hit = std::numeric_limits<float>::max();
        Vec3 intersection, n;
        bool intersects = false;

        // FIXME: Bounds check to get a shortlist!
        for(auto& tri: triangles) {
            float dist = closest_hit;
            Vec3 intersect;
            bool hit = kmRay3IntersectTriangle(
                &ray,
                &vertices[tri.index[0]],
                &vertices[tri.index[1]],
                &vertices[tri.index[2]], &intersect, &n, &dist
            );

            if(hit && dist < closest_hit) {
                closest_hit = dist;
                intersection = intersect;
                intersects = true;
            }
        }

        if(hit_distance) {
            *hit_distance = closest_hit;
        }

        return std::make_pair(intersection, intersects);
    }
};


class RigidBody;

class RigidBodySimulation:
    public Managed<RigidBodySimulation> {

public:
    RigidBodySimulation();
    bool init() override;
    void cleanup() override;

    void step(double dt);

    std::pair<Vec3, bool> intersect_ray(const Vec3& start, const Vec3& direction, float* distance=nullptr);

private:
    friend class impl::Body;
    friend class RigidBody;
    friend class StaticBody;

    q3Scene* scene_ = nullptr;

    // Used by the RigidBodyController on creation/destruction to register a body
    // in the simulation
    q3Body *acquire_body(impl::Body* body);
    void release_body(impl::Body *body);

    std::unordered_map<impl::Body*, q3Body*> bodies_;

    std::pair<Vec3, Quaternion> body_transform(impl::Body* body);
    void set_body_transform(impl::Body *body, const Vec3& position, const Quaternion& rotation);

    std::unordered_map<impl::Body*, RaycastCollider> raycast_colliders_;
};

enum ColliderType {
    /* OOB collisiion detection */
    COLLIDER_TYPE_BOX,

     /* Useful for terrain meshes. Doesn't move or interact with other colliders but
     * allows for manual collider logic. */
    COLLIDER_TYPE_RAYCAST_ONLY
};

namespace impl {
    class Body:
        public Controller {

    public:
        Body(Controllable* object, RigidBodySimulation::ptr simulation, ColliderType collider=COLLIDER_TYPE_BOX);
        virtual ~Body();

        void move_to(const Vec3& position);
        void rotate_to(const Quaternion& rotation);

        bool init();
        void cleanup();

    protected:
        friend class kglt::controllers::RigidBodySimulation;
        MoveableObject* object_;
        q3Body* body_ = nullptr;
        RigidBodySimulation::ptr simulation_;
        ColliderType collider_type_;

        void do_post_update(double dt) override;

        void build_collider(ColliderType collider);

    private:
        virtual bool is_dynamic() const { return true; }
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

    using impl::Body::init;
    using impl::Body::cleanup;
private:
    bool is_dynamic() const override { return false; }
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

    using impl::Body::init;
    using impl::Body::cleanup;
private:
    friend class RigidBodySimulation;

    // Cleared each step
    Vec3 force_;
    Vec3 torque_;
};

}
}
