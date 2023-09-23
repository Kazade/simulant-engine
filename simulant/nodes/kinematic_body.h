#pragma once

#include "dynamic_physics_body.h"

namespace smlt {

struct KinematicBodyParams {};

class KinematicBody:
    public StageNode,
    public DynamicPhysicsBody {
public:
    struct Meta {
        const static StageNodeType node_type = STAGE_NODE_TYPE_PHYSICS_KINEMATIC_BODY;
        typedef KinematicBodyParams params_type;
    };
};

}
