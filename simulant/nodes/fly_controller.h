#pragma once

#include "simulant/utils/construction_args.h"
#include "stage_node.h"

namespace smlt {

class FlyController;

class FlyController: public StageNode {
private:
    float speed_ = 600.0f;

public:
    S_DEFINE_STAGE_NODE_META(STAGE_NODE_TYPE_FLY_CONTROLLER);

    FlyController(Scene* owner) :
        StageNode(owner, STAGE_NODE_TYPE_FLY_CONTROLLER) {}

    void set_speed(float v) {
        speed_ = v;
    }

    float speed() const {
        return speed_;
    }

    void on_late_update(float dt) override;

    const AABB& aabb() const override {
        return AABB::ZERO;
    }

private:
    bool on_create(const Args& params) override {
        set_speed(params.arg<float>("speed").value_or(600.0f));
        return true;
    }
};

} // namespace smlt
