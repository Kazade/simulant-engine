#pragma once

#include "../scenes/scene.h"
#include "debug.h"
#include "locators/node_locator.h"
#include "reactive_body.h"
#include "stage_node.h"

namespace smlt {

class Scene;

class DynamicBody: public ReactiveBody {

public:
    FindResult<Debug> debug = smlt::FindChild<Debug>(this);

    S_DEFINE_STAGE_NODE_META(STAGE_NODE_TYPE_PHYSICS_DYNAMIC_BODY,
                             "dynamic_body");
    S_DEFINE_CORE_PHYSICS_BODY_PROPERTIES(DynamicBody);

    DynamicBody(Scene* owner) :
        ReactiveBody(owner, STAGE_NODE_TYPE_PHYSICS_DYNAMIC_BODY,
                     PHYSICS_BODY_TYPE_DYNAMIC) {}

    const AABB& aabb() const override {
        static AABB aabb;
        return aabb;
    }

private:
    bool on_create(Params params) override {
        if(!clean_params<DynamicBody>(params)) {
            return false;
        }
        return PhysicsBody::on_create(params);
    }
};

} // namespace smlt
