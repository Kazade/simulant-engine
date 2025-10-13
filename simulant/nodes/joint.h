#pragma once

#include "../generic/optional.h"
#include "stage_node.h"

namespace smlt {

class AnimationJoint: public StageNode {
public:
    S_DEFINE_STAGE_NODE_META(STAGE_NODE_TYPE_JOINT, "joint");
    S_DEFINE_STAGE_NODE_PARAM(AnimationJoint, "inverse_bind_matrix", FloatArray,
                              no_value,
                              "The inverse bind matrix for this joint");

    AnimationJoint(smlt::Scene* owner) :
        StageNode(owner, Meta::node_type) {}

    bool on_create(Params params) override {
        if(!clean_params<AnimationJoint>(params)) {
            return false;
        }

        return StageNode::on_create(params);
    }

private:
    Mat4 inverse_bind_matrix_;
};

} // namespace smlt
