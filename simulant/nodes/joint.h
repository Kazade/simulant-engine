#pragma once

#include "stage_node.h"

namespace smlt {

class AnimationJoint: public StageNode {
public:
    S_DEFINE_STAGE_NODE_META(STAGE_NODE_TYPE_JOINT, "joint");

    AnimationJoint(smlt::Scene* owner) :
        StageNode(owner, Meta::node_type) {}

    bool on_create(Params params) override {
        return StageNode::on_create(params);
    }
};

} // namespace smlt
