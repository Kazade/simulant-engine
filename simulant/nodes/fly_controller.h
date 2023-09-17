#pragma once

#include "stage_node.h"

namespace smlt {

class FlyController;

struct FlyControllerParams {};

template<>
struct stage_node_traits<FlyController> {
    const static StageNodeType node_type = STAGE_NODE_TYPE_FLY_CONTROLLER;
    typedef FlyControllerParams params_type;
};

class FlyController : public StageNode {
private:
    float speed_ = 600.0f;

public:
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
};

}
