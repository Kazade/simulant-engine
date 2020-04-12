#pragma once

#include "../behaviour.h"
#include "../stage_node_behaviour.h"
#include "../../generic/managed.h"

namespace smlt {
namespace behaviours {

class CylindricalBillboard:
    public StageNodeBehaviour,
    public RefCounted<CylindricalBillboard> {

public:
    CylindricalBillboard(StageNode* target):
        target_(target) {}

    void set_target(StageNode* target) {
        target_ = target;
    }

    void late_update(float dt) override {
        _S_UNUSED(dt);

        if(target_) {
            auto dir = (
                target_->absolute_position() - stage_node_->absolute_position()
            );

            smlt::Vec3 up_plane(smlt::Vec3::POSITIVE_Y, 0);
            dir = up_plane.project(dir).normalized();

            stage_node_->rotate_to_absolute(
                smlt::Vec3::NEGATIVE_Z.rotation_to(dir)
            );
        }
    }
private:
    StageNode* target_ = nullptr;
};

}
}
