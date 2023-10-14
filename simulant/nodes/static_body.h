#pragma once

#include "physics_body.h"
#include "stage_node.h"

#include "../scenes/scene.h"

namespace smlt {

struct StaticBodyParams : public PhysicsBodyParams {
    StaticBodyParams(const Vec3& position=Vec3(), const Quaternion& rotation=Quaternion()):
        PhysicsBodyParams(position, rotation) {}
};

class StaticBody:
    public PhysicsBody {

public:
    struct Meta {
        const static StageNodeType node_type = STAGE_NODE_TYPE_PHYSICS_STATIC_BODY;
        typedef StaticBodyParams params_type;
    };

    StaticBody(Scene* owner):
        PhysicsBody(owner, STAGE_NODE_TYPE_PHYSICS_STATIC_BODY, PHYSICS_BODY_TYPE_STATIC) {}

    void add_mesh_collider(
        const MeshPtr& mesh,
        const PhysicsMaterial& properties,
        uint16_t kind=0,
        const Vec3& offset=Vec3(), const Quaternion& rotation=Quaternion()
    );

    const AABB& aabb() const override {
        static AABB aabb;
        return aabb;
    }

private:
    bool on_create(void *params) override;
};

}
