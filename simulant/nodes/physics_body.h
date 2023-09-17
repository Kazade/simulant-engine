#pragma once

#include <vector>
#include <unordered_map>

#include "../types.h"

namespace smlt {

class StageNode;

struct PhysicsMaterial {
    PhysicsMaterial() = default;
    PhysicsMaterial(float density, float friction, float bounciness):
        density(density), friction(friction), bounciness(bounciness) {}

    float density = 0.0f;
    float friction = 0.0f;
    float bounciness = 0.0f;

    static const PhysicsMaterial WOOD;
    static const PhysicsMaterial RUBBER;
    static const PhysicsMaterial IRON;
    static const PhysicsMaterial STONE;

    static const PhysicsMaterial WOOD_25;
    static const PhysicsMaterial WOOD_50;
    static const PhysicsMaterial WOOD_75;

    static const PhysicsMaterial RUBBER_25;
    static const PhysicsMaterial RUBBER_50;
    static const PhysicsMaterial RUBBER_75;

    static const PhysicsMaterial IRON_25;
    static const PhysicsMaterial IRON_50;
    static const PhysicsMaterial IRON_75;

    static const PhysicsMaterial STONE_25;
    static const PhysicsMaterial STONE_50;
    static const PhysicsMaterial STONE_75;
};

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
    std::string other_collider_name; ///< Name of the collider we hit
    StageNode* other_stage_node = nullptr; ///< Pointer to the owning stage node of the other body

    PhysicsBody* this_body = nullptr; ///< This body
    std::string this_collider_name; ///< The collider on this body which was hit
    StageNode* this_stage_node = nullptr; ///< The owning stage node of this body

    std::vector<ContactPoint> contact_points; ///< List of contact points found during this collision
};

class CollisionListener;

class PhysicsBody {
public:
    PhysicsBody(StageNode* self):
        self_(self) {}

    void add_box_collider(
        const Vec3& size,
        const PhysicsMaterial& properties,
        uint16_t kind=0,
        const Vec3& offset=Vec3(), const Quaternion& rotation=Quaternion()
    );

    void add_sphere_collider(
        const float diameter,
        const PhysicsMaterial& properties,
        uint16_t kind=0,
        const Vec3& offset=Vec3()
    );

    void add_capsule_collider(
        float height,
        const float diameter,
        const PhysicsMaterial& properties,
        uint16_t kind=0
    );

    void add_triangle_collider(
        const smlt::Vec3& v1, const smlt::Vec3& v2, const smlt::Vec3& v3,
        const PhysicsMaterial& properties,
        uint16_t kind=0
    );

    void register_collision_listener(CollisionListener* listener);
    void unregister_collision_listener(CollisionListener* listener);

private:
    StageNode* self_ = nullptr;
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

}
