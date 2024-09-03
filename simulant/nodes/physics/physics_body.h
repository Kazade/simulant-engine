#pragma once

#include <set>
#include <unordered_map>
#include <vector>

#include "../../types.h"
#include "../stage_node.h"
#include "contact_list.h"
#include "material.h"
#include "simulant/utils/params.h"

#define S_DEFINE_CORE_PHYSICS_BODY_PROPERTIES(klass)                           \
    TypedNodeParam<FloatArray, klass> param_10000 = {                          \
        10000, "position", smlt::Vec3(), "The initial position of the body"};  \
    TypedNodeParam<FloatArray, klass> param_10001 = {                          \
        10001, "orientation", smlt::Quaternion(),                              \
        "The initial rotation of the body"}

namespace smlt {

namespace _impl {
struct BounceData;
}

class StageNode;
class PhysicsService;

class PhysicsBody;

struct ContactPoint {
    Vec3 normal;
    Vec3 point;

    float separation = 0.0f;

    PhysicsBody* other_body = nullptr;
    std::string other_collider;
};

struct Collision {
    PhysicsBody* other_body = nullptr; ///< Body which owns the collider we hit
    std::string other_collider_name;   ///< Name of the collider we hit

    PhysicsBody* this_body = nullptr; ///< This body
    std::string this_collider_name; ///< The collider on this body which was hit

    std::vector<ContactPoint>
        contact_points; ///< List of contact points found during this collision
};

class CollisionListener;

enum PhysicsBodyType {
    PHYSICS_BODY_TYPE_STATIC,
    PHYSICS_BODY_TYPE_DYNAMIC,
    PHYSICS_BODY_TYPE_KINEMATIC,
};

class Scene;

struct PhysicsBodyParams {
    Vec3 initial_position;
    Quaternion initial_rotation;

    PhysicsBodyParams(const Vec3& position = Vec3(),
                      const Quaternion& rotation = Quaternion()) :
        initial_position(position), initial_rotation(rotation) {}
};

class PhysicsBody: public StageNode {
public:
    PhysicsBody(Scene* owner, StageNodeType node_type, PhysicsBodyType type);

    virtual ~PhysicsBody();

    void add_box_collider(const Vec3& size, const PhysicsMaterial& properties,
                          uint16_t kind = 0, const Vec3& offset = Vec3(),
                          const Quaternion& rotation = Quaternion());

    void add_sphere_collider(const float diameter,
                             const PhysicsMaterial& properties,
                             uint16_t kind = 0, const Vec3& offset = Vec3());

    void add_capsule_collider(const Vec3& v0, const Vec3& v1,
                              const float diameter,
                              const PhysicsMaterial& properties,
                              uint16_t kind = 0);

    void add_triangle_collider(const smlt::Vec3& v1, const smlt::Vec3& v2,
                               const smlt::Vec3& v3,
                               const PhysicsMaterial& properties,
                               uint16_t kind = 0);

    void register_collision_listener(CollisionListener* listener);
    void unregister_collision_listener(CollisionListener* listener);

    PhysicsBodyType type() const {
        return type_;
    }

    Vec3 position() const;
    Quaternion orientation() const;

    void set_position(const Vec3& position);
    void set_orientation(const Quaternion& rotation);

protected:
    friend class PhysicsService;
    friend class ContactListIterator;
    friend class _PhysicsData;

    PhysicsService* get_simulation() const;

    bool on_create(Params params) override;
    bool on_destroy() override;

    void on_transformation_changed() override;

    std::unique_ptr<_impl::BounceData> bounce_;

private:
    friend class ContactListener;

    bool updating_body_ = false;

    std::pair<Vec3, Quaternion> last_state_;
    void on_update(float dt) override;

    PhysicsBodyType type_;
    std::set<CollisionListener*> listeners_;

    void contact_started(const Collision& collision);
    void contact_finished(const Collision& collision);

    ContactList contact_list_ = ContactList(this);

public:
    ContactList& contacts() {
        return contact_list_;
    }
};

class CollisionListener {
public:
    virtual ~CollisionListener() {
        auto watching = watching_;
        for(auto body: watching) {
            body->unregister_collision_listener(this);
        }
    }

private:
    friend class PhysicsBody;

    virtual void on_collision_enter(const Collision& collision) {
        _S_UNUSED(collision);
    }
    virtual void on_collision_stay() {}
    virtual void on_collision_exit(const Collision& collision) {
        _S_UNUSED(collision);
    }

    virtual void on_trigger_enter() {}
    virtual void on_trigger_stay() {}
    virtual void on_trigger_exit() {}

    std::unordered_set<PhysicsBody*> watching_;
};

} // namespace smlt
