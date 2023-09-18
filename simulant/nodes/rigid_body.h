#pragma once

#include "dynamic_physics_body.h"

namespace smlt {

struct RigidBodyParams {};

class RigidBody:
    public StageNode,
    public DynamicPhysicsBody {

};

template<>
struct stage_node_traits<RigidBody> {
    const static StageNodeType node_type = STAGE_NODE_TYPE_PHYSICS_RIGID_BODY;
    typedef RigidBodyParams params_type;
};


}
