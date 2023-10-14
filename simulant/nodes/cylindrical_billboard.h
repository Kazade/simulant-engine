#pragma once

#include "stage_node.h"

namespace smlt {

struct CylindricalBillboardParams {
    Vec3 forward;
    CylindricalBillboardParams(const Vec3& forward=Vec3::NEGATIVE_Z):
        forward(forward) {}
};

class CylindricalBillboard:
    public StageNode {

public:
    struct Meta {
        const static StageNodeType node_type = STAGE_NODE_TYPE_CYLINDRICAL_BILLBOARD;
        typedef CylindricalBillboardParams params_type;
    };

    CylindricalBillboard(Scene* owner):
        StageNode(owner, STAGE_NODE_TYPE_CYLINDRICAL_BILLBOARD) {}

    void set_target(StageNode* target) {
        target_ = target;
    }

    const AABB& aabb() const {
        static AABB aabb;
        return aabb;
    }
private:
    StageNode* target_ = nullptr;
    Vec3 forward_;

    bool on_create(void* params) {
        CylindricalBillboardParams* args = (CylindricalBillboardParams*) params;
        forward_ = args->forward;
        return true;
    }

    void on_late_update(float dt) override {
        _S_UNUSED(dt);

        if(target_) {
            auto dir = (
                target_->transform->position() - transform->position()
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
            transform->set_orientation(rot);
        }
    }

};

}
