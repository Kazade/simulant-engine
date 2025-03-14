#pragma once

#include "simulant/utils/params.h"
#include "stage_node.h"

namespace smlt {

class CylindricalBillboard: public StageNode {

public:
    S_DEFINE_STAGE_NODE_META(STAGE_NODE_TYPE_CYLINDRICAL_BILLBOARD,
                             "cylindrical_billboard");
    S_DEFINE_STAGE_NODE_PARAM(CylindricalBillboard, "forward", FloatArray,
                              Vec3(0, 0, -1),
                              "The forward direction of the billboard");

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

    bool on_create(Params params) override {
        if(!clean_params<CylindricalBillboard>(params)) {
            return false;
        }

        forward_ = params.get<FloatArray>("forward").value_or(Vec3::forward());
        return true;
    }

    void on_late_update(float dt) override {
        _S_UNUSED(dt);

        if(target_) {
            auto dir = (target_->transform->position() - transform->position())
                           .normalized();

            smlt::Plane up_plane(smlt::Vec3::up(), 0);

            /* If we're right above/below the Y axis then default to
             * looking down negative Z */
            auto d = std::abs(dir.dot(Vec3::up()));
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
