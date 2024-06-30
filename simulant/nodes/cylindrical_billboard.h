#pragma once

#include "simulant/utils/construction_args.h"
#include "stage_node.h"

namespace smlt {

class CylindricalBillboard: public StageNode {

public:
    S_DEFINE_STAGE_NODE_META(STAGE_NODE_TYPE_CYLINDRICAL_BILLBOARD);

    CylindricalBillboard(Scene* owner) :
        StageNode(owner, STAGE_NODE_TYPE_CYLINDRICAL_BILLBOARD) {}

    void set_target(StageNode* target) {
        target_ = target;
    }

    const AABB& aabb() const override {
        static AABB aabb;
        return aabb;
    }

private:
    StageNode* target_ = nullptr;
    Vec3 forward_;

    bool on_create(const Params& params) override {
        forward_ = params.arg<Vec3>("forward").value_or(Vec3::FORWARD);
        return true;
    }

    void on_late_update(float dt) override {
        _S_UNUSED(dt);

        if(target_) {
            auto dir = (target_->transform->position() - transform->position())
                           .normalized();

            smlt::Plane up_plane(smlt::Vec3::POSITIVE_Y, 0);

            /* If we're right above/below the Y axis then default to
             * looking down negative Z */
            auto d = std::abs(dir.dot(Vec3::POSITIVE_Y));
            if(almost_equal(d, 1.0f)) {
                dir = forward_;
            }

            dir = up_plane.project(dir).normalized();

            auto rot = forward_.rotation_to(dir);
            transform->set_orientation(rot);
        }
    }
};

} // namespace smlt
