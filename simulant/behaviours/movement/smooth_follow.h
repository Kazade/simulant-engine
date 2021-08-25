#pragma once

#include "../../types.h"
#include "../stage_node_behaviour.h"


namespace smlt {

class Organism;

namespace behaviours {

class SmoothFollow:
    public StageNodeBehaviour,
    public RefCounted<SmoothFollow> {

public:
    SmoothFollow();
    ~SmoothFollow();

    const char* name() const override {
        return "Smooth Follow";
    }

    void late_update(float dt) override;
    void set_target(ActorPtr actor);
    void set_target(ParticleSystemPtr ps);
    bool has_target() const;
    StageNode* target() const;

    void set_follow_distance(float dist) { distance_ = dist; }
    void set_follow_height(float height) { height_ = height; }
    void set_damping(float damping);
    void set_rotation_damping(float damping);

    /* When set to false, the node will stay focused
     * on the target, but movement will stop */
    void set_following_enabled(bool v);
private:
    StageNode* target_ = nullptr;

    bool following_enabled_ = true;

    float distance_ = 10.0;
    float height_ = 10.0;

    float damping_ = 5.0;
    float rotation_damping_ = 10.0;

    sig::Connection destroy_conn_;

};

}
}
