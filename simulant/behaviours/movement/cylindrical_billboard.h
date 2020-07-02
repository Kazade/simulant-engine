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
    CylindricalBillboard(StageNode* target, const Vec3& forward=Vec3::NEGATIVE_Z):
        target_(target),
        forward_(forward) {}

    void set_target(StageNode* target) {
        target_ = target;
    }

    void late_update(float dt) override {
        _S_UNUSED(dt);

        if(target_) {
            auto dir = (
                target_->absolute_position() - stage_node->absolute_position()
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
            stage_node->rotate_to_absolute(rot);
        }
    }

    const std::string name() const override {
        return "cylindrical_billboard";
    }
private:
    StageNode* target_ = nullptr;
    Vec3 forward_;
};

}
}
