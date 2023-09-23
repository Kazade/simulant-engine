#pragma once

#include "physics_body.h"
#include "stage_node.h"

namespace smlt {

class StaticBodyParams {};

class StaticBody:
    public StageNode,
    public PhysicsBody {

public:
    struct Meta {
        const static StageNodeType node_type = STAGE_NODE_TYPE_PHYSICS_STATIC_BODY;
        typedef StaticBodyParams params_type;
    };

    void add_mesh_collider(
        const MeshPtr& mesh,
        const PhysicsMaterial& properties,
        uint16_t kind=0,
        const Vec3& offset=Vec3(), const Quaternion& rotation=Quaternion()
    );
};

}
