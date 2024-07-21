#pragma once

#include "physics_body.h"
#include "stage_node.h"

#include "../scenes/scene.h"

namespace smlt {

class StaticBody: public PhysicsBody {

public:
    S_DEFINE_STAGE_NODE_META(STAGE_NODE_TYPE_PHYSICS_STATIC_BODY,
                             "static_body");
    S_DEFINE_CORE_PHYSICS_BODY_PROPERTIES(StaticBody);

    S_DEFINE_STAGE_NODE_PARAM(StaticBody, "mesh", MeshPtr, no_value,
                              "Source mesh for a mesh collider");

    S_DEFINE_STAGE_NODE_PARAM(StaticBody, "density", float, 0.1f,
                              "Density of the static body material");

    S_DEFINE_STAGE_NODE_PARAM(StaticBody, "friction", float, 0.2f,
                              "Friction of the static body material");

    S_DEFINE_STAGE_NODE_PARAM(StaticBody, "bounciness", float, 0.00001f,
                              "Bounciness of the static body material");

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
    bool on_create(Params params) override;
};

} // namespace smlt
