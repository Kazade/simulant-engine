#pragma once

#include <unordered_map>
#include <utility>
#include <set>

#include "collider.h"

#include "../behaviour.h"
#include "../stage_node_behaviour.h"
#include "../../generic/property.h"
#include "../../types.h"

struct b3World;
struct b3Body;
struct b3Hull;
struct b3Mesh;
struct b3MeshTriangle;
struct b3Vec3;
struct b3Shape;

namespace smlt {

namespace utils {
    struct Triangle;
}

class StageNode;

namespace behaviours {

namespace impl {
class Body;
}

class RigidBodySimulation;
class CollisionListener;

struct ContactPoint {
    Vec3 normal;
    Vec3 point;

    float separation = 0.0f;

    impl::Body* other_body = nullptr;
    std::string other_collider;
};

struct Collision {
    impl::Body* other_body = nullptr; ///< Body which owns the collider we hit
    std::string other_collider_name; ///< Name of the collider we hit
    StageNode* other_stage_node = nullptr; ///< Pointer to the owning stage node of the other body

    impl::Body* this_body = nullptr; ///< This body
    std::string this_collider_name; ///< The collider on this body which was hit
    StageNode* this_stage_node = nullptr; ///< The owning stage node of this body

    std::vector<ContactPoint> contact_points; ///< List of contact points found during this collision
};

namespace impl {

class ContactListener;

class Body:
    public StageNodeBehaviour {

public:
    Body(RigidBodySimulation* simulation);
    virtual ~Body();

    void move_to(const Vec3& position);
    void rotate_to(const Quaternion& rotation);

    bool init();
    void clean_up();

    void add_box_collider(
        const Vec3& size,
        const PhysicsMaterial& properties,
        const Vec3& offset=Vec3(), const Quaternion& rotation=Quaternion()
    );

    void add_sphere_collider(const float diameter,
        const PhysicsMaterial& properties,
        const Vec3& offset=Vec3()
    );

    void add_capsule_collider(
        const Vec3& v1,
        const Vec3& v2,
        const float diameter,
        const PhysicsMaterial& properties
    );

    void register_collision_listener(CollisionListener* listener);
    void unregister_collision_listener(CollisionListener* listener);

    RigidBodySimulation* _simulation_ptr() const {
        if(auto ret = simulation_.lock()) {
            return ret.get();
        } else {
            return nullptr;
        }
    }

protected:
    friend class smlt::behaviours::RigidBodySimulation;

    b3Body* body_ = nullptr;
    std::weak_ptr<RigidBodySimulation> simulation_;

    std::pair<Vec3, Quaternion> last_state_;

    void update(float dt) override;

    struct ColliderDetails {
        PhysicsMaterial material;
        std::string name;
    };

    void store_collider(b3Shape* shape, const PhysicsMaterial& material);

    std::unordered_map<b3Shape*, ColliderDetails> collider_details_;

private:
    virtual bool is_dynamic() const { return true; }
    virtual bool is_kinematic() const { return false; }

    sig::connection simulation_stepped_connection_;
    std::vector<std::shared_ptr<b3Hull>> hulls_;
    std::set<CollisionListener*> listeners_;

    friend class impl::ContactListener;

    void contact_started(const Collision& collision);
    void contact_finished(const Collision &collision);

    void on_behaviour_added(Organism* organism) override;

public:
    Property<decltype(&Body::simulation_)> simulation = {
        this, &Body::simulation_
    };
};

} // End impl




}
}
