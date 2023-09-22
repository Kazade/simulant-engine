#pragma once

#include "stage_node.h"

namespace smlt {

class SphericalBillboard:
    public StageNode {

public:
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

            auto rot = smlt::Vec3::NEGATIVE_Z.rotation_to(dir);
            rotate_to_absolute(rot);
        }
    }
};

}
