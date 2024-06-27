#pragma once

#include "physics_body.h"
#include "stage_node.h"

#include "../scenes/scene.h"

namespace smlt {

class StaticBody: public PhysicsBody {

public:
    S_DEFINE_STAGE_NODE_META(STAGE_NODE_TYPE_PHYSICS_STATIC_BODY);

    StaticBody(Scene* owner) :
        PhysicsBody(owner, STAGE_NODE_TYPE_PHYSICS_STATIC_BODY,
                    PHYSICS_BODY_TYPE_STATIC) {}

    void add_mesh_collider(const MeshPtr& mesh,
                           const PhysicsMaterial& properties, uint16_t kind = 0,
                           const Vec3& offset = Vec3(),
                           const Quaternion& rotation = Quaternion());

    const AABB& aabb() const override {
        static AABB aabb;
        return aabb;
    }

private:
    bool on_create(const ConstructionArgs& params) override;
};

} // namespace smlt
