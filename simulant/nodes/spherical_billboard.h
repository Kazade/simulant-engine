#pragma once

#include "simulant/utils/params.h"
#include "stage_node.h"

namespace smlt {

class SphericalBillboard: public StageNode {

public:
    S_DEFINE_STAGE_NODE_META(STAGE_NODE_TYPE_SPHERICAL_BILLBOARD,
                             "spherical_billboard");

    SphericalBillboard(Scene* owner) :
        StageNode(owner, STAGE_NODE_TYPE_SPHERICAL_BILLBOARD) {}

    void set_target(StageNode* target) {
        target_ = target;
    }

    const AABB& aabb() const override {
        static AABB aabb;
        return aabb;
    }

private:
    bool on_create(Params params) override {
        forward_ = params.get<FloatArray>("forward").value_or(Vec3::forward());
        return true;
    }

    StageNode* target_ = nullptr;
    Vec3 forward_;

    void on_late_update(float dt) override {
        _S_UNUSED(dt);

        if(target_) {
            auto dir = (target_->transform->position() - transform->position())
                           .normalized();

            auto rot = forward_.rotation_to(dir);
            transform->set_orientation(rot);
        }
    }
};

} // namespace smlt
