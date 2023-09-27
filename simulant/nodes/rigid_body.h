#pragma once

#include "dynamic_physics_body.h"
#include "../scenes/scene.h"

namespace smlt {

class Scene;
struct RigidBodyParams {};

class RigidBody:
    public StageNode,
    public DynamicPhysicsBody {

public:
    struct Meta {
        const static StageNodeType node_type = STAGE_NODE_TYPE_PHYSICS_RIGID_BODY;
        typedef RigidBodyParams params_type;
    };

    RigidBody(Scene* owner):
        StageNode(owner, STAGE_NODE_TYPE_PHYSICS_RIGID_BODY),
        DynamicPhysicsBody(owner, PHYSICS_BODY_TYPE_DYNAMIC) {}

    const AABB& aabb() const override {
        static AABB aabb;
        return aabb;
    }

private:
    bool on_create(void* params) override {
        return true;
    }
};

}
