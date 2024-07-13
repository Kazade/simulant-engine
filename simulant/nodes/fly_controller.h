#pragma once

#include "simulant/utils/params.h"
#include "stage_node.h"

namespace smlt {

class FlyController;

class FlyController: public StageNode {
private:
    float speed_ = 600.0f;

public:
    S_DEFINE_STAGE_NODE_META(STAGE_NODE_TYPE_FLY_CONTROLLER, "fly");
    S_DEFINE_STAGE_NODE_PARAM(FlyController, "speed", float, 600.0f,
                              "The speed at which the controller moves");
    FlyController(Scene* owner) :
        StageNode(owner, STAGE_NODE_TYPE_FLY_CONTROLLER) {}

    void set_speed(float v) {
        speed_ = v;
    }

    float speed() const {
        return speed_;
    }

    void on_late_update(float dt) override;

private:
    bool on_create(Params params) override {
        if(!clean_params<FlyController>(params)) {
            return false;
        }
        set_speed(params.get<float>("speed").value_or(600.0f));
        return true;
    }
};

} // namespace smlt
