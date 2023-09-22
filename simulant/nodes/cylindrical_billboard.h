#pragma once

#include "stage_node.h"

namespace smlt {

class CylindricalBillboard:
    public StageNode {

public:
    CylindricalBillboard(Scene* owner):
        StageNode(owner, STAGE_NODE_TYPE_CYLINDRICAL_BILLBOARD) {}

    void set_target(StageNode* target) {
        target_ = target;
    }

private:
    StageNode* target_ = nullptr;
    Vec3 forward_;

    void on_late_update(float dt) override {
        _S_UNUSED(dt);

        if(target_) {
            auto dir = (
                target_->absolute_position() - absolute_position()
            ).normalized();

            smlt::Plane up_plane(smlt::Vec3::POSITIVE_Y, 0);

            /* If we're right above/below the Y axis then default to
             * looking down negative Z */
            auto d = std::abs(dir.dot(Vec3::POSITIVE_Y));
            if(almost_equal(d, 1.0f)) {
                dir = forward_;
            }

            dir = up_plane.project(dir).normalized();

            auto rot = forward_.rotation_to(dir);
            rotate_to_absolute(rot);
        }
    }

};

}
