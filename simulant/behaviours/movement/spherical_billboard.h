#pragma once

#include "../behaviour.h"
#include "../stage_node_behaviour.h"
#include "../../generic/managed.h"

namespace smlt {
namespace behaviours {

class SphericalBillboard:
    public StageNodeBehaviour,
    public RefCounted<SphericalBillboard> {

public:
    SphericalBillboard(StageNode* target):
        target_(target) {}

    void set_target(StageNode* target) {
        target_ = target;
    }

    void late_update(float dt) override {
        _S_UNUSED(dt);

        if(target_) {
            auto dir = (
                target_->absolute_position() - stage_node->absolute_position()
            ).normalized();

            auto rot = smlt::Vec3::NEGATIVE_Z.rotation_to(dir);
            stage_node->rotate_to_absolute(rot);
        }
    }

    const std::string name() const override {
        return "spherical_billboard";
    }
private:
    StageNode* target_ = nullptr;
};

}
}
