#pragma once

#include "stage_node.h"

namespace smlt {

struct SphericalBillboardParams {};

class SphericalBillboard:
    public StageNode {

public:
    struct Meta {
        const static StageNodeType node_type = STAGE_NODE_TYPE_SPHERICAL_BILLBOARD;
        typedef SphericalBillboardParams params_type;
    };

    SphericalBillboard(Scene* owner):
        StageNode(owner, STAGE_NODE_TYPE_SPHERICAL_BILLBOARD) {}

    void set_target(StageNode* target) {
        target_ = target;
    }

    const AABB& aabb() const {
        static AABB aabb;
        return aabb;
    }
private:
    bool on_create(void*) {
        return true;
    }

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