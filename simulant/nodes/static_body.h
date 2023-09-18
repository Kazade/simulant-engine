#pragma once

#include "physics_body.h"
#include "stage_node.h"

namespace smlt {

class StaticBodyParams {};

class StaticBody:
    public StageNode,
    public PhysicsBody {

};

template<>
struct stage_node_traits<StaticBody> {
    const static StageNodeType node_type = STAGE_NODE_TYPE_PHYSICS_STATIC_BODY;
    typedef StaticBodyParams params_type;
};

}
