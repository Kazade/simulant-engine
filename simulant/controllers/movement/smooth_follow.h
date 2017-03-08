#pragma once

#include "../../types.h"
#include "../stage_node_controller.h"


namespace smlt {
namespace controllers {

class SmoothFollow:
    public StageNodeController {

public:
    const std::string name() const {
        return "Smooth Follow";
    }

    void late_update(double dt) override;
    void set_target(ActorPtr actor);
    void set_target(ParticleSystemPtr ps);

private:
    std::weak_ptr<StageNode> target_;

    float distance_ = 10.0;
    float height_ = 10.0;

    float height_damping_ = 2.0;
    float rotation_damping_ = 0.6;

};

}
}
