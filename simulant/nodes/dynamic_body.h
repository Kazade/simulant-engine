#pragma once

#include "reactive_body.h"
#include "../scenes/scene.h"

namespace smlt {

class Scene;
struct DynamicBodyParams : public PhysicsBodyParams {
    DynamicBodyParams(const Vec3& position=Vec3(), const Quaternion& rotation=Quaternion()):
        PhysicsBodyParams(position, rotation) {}
};

class DynamicBody:
    public ReactiveBody {

public:
    struct Meta {
        const static StageNodeType node_type = STAGE_NODE_TYPE_PHYSICS_DYNAMIC_BODY;
        typedef DynamicBodyParams params_type;
    };

    DynamicBody(Scene* owner):
        ReactiveBody(owner, STAGE_NODE_TYPE_PHYSICS_DYNAMIC_BODY, PHYSICS_BODY_TYPE_DYNAMIC) {}

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
