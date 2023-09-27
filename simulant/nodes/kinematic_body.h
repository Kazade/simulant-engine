#pragma once

#include "dynamic_physics_body.h"
#include "../scenes/scene.h"

namespace smlt {

class Scene;
struct KinematicBodyParams : public PhysicsBodyParams {
    KinematicBodyParams(const Vec3& position=Vec3(), const Quaternion& rotation=Quaternion()):
        PhysicsBodyParams(position, rotation) {}
};

class KinematicBody:
    public DynamicPhysicsBody {

public:
    struct Meta {
        const static StageNodeType node_type = STAGE_NODE_TYPE_PHYSICS_KINEMATIC_BODY;
        typedef KinematicBodyParams params_type;
    };

    KinematicBody(Scene* owner):
        DynamicPhysicsBody(owner, STAGE_NODE_TYPE_PHYSICS_KINEMATIC_BODY, PHYSICS_BODY_TYPE_KINEMATIC) {}

    const AABB& aabb() const override {
        static AABB aabb;
        return aabb;
    }

private:
    bool on_create(void* params) override {
        return PhysicsBody::on_create(params);
    }
};

}
