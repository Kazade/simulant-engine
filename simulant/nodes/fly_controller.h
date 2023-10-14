#pragma once

#include "stage_node.h"

namespace smlt {

class FlyController;

struct FlyControllerParams {
    float speed;
    FlyControllerParams(float speed=600.0f):
        speed(speed) {}
};


class FlyController : public StageNode {
private:
    float speed_ = 600.0f;

public:
    struct Meta {
        const static StageNodeType node_type = STAGE_NODE_TYPE_FLY_CONTROLLER;
        typedef FlyControllerParams params_type;
    };

    FlyController(Scene* owner):
        StageNode(owner, STAGE_NODE_TYPE_FLY_CONTROLLER) {

    }

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
    bool on_create(void *params) override {
        FlyControllerParams* args = (FlyControllerParams*) params;
        set_speed(args->speed);
        return true;
    }
};

}
