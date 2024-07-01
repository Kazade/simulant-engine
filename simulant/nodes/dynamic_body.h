#pragma once

#include "../scenes/scene.h"
#include "reactive_body.h"
#include "simulant/nodes/stage_node.h"

namespace smlt {

class Scene;

class DynamicBody: public ReactiveBody {

public:
    S_DEFINE_STAGE_NODE_META(STAGE_NODE_TYPE_PHYSICS_DYNAMIC_BODY);

    DynamicBody(Scene* owner) :
        ReactiveBody(owner, STAGE_NODE_TYPE_PHYSICS_DYNAMIC_BODY,
                     PHYSICS_BODY_TYPE_DYNAMIC) {}

    const AABB& aabb() const override {
        static AABB aabb;
        return aabb;
    }

private:
    bool on_create(Params params) override {
        return PhysicsBody::on_create(params);
    }
};

} // namespace smlt
