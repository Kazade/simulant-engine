#pragma once

#include "../../scenes/scene.h"
#include "../stage_node.h"
#include "reactive_body.h"

namespace smlt {

class Scene;

class KinematicBody: public ReactiveBody {

public:
    S_DEFINE_STAGE_NODE_META("kinematic_body");
    S_DEFINE_CORE_PHYSICS_BODY_PROPERTIES(KinematicBody);

    KinematicBody(Scene* owner) :
        ReactiveBody(owner, Meta::node_type, PHYSICS_BODY_TYPE_KINEMATIC) {}

    const AABB& aabb() const override {
        static AABB aabb;
        return aabb;
    }

private:
    bool on_create(Params params) override {
        if(!clean_params<KinematicBody>(params)) {
            return false;
        }

        return PhysicsBody::on_create(params);
    }
};

} // namespace smlt
