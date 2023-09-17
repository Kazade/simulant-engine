#pragma once

#include "physics_body.h"
#include "stage_node.h"

namespace smlt {

class StaticBody:
    public StageNode,
    public PhysicsBody {

};

}
