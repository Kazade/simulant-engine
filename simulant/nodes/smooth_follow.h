#pragma once

#include "stage_node.h"

namespace smlt {

class SmoothFollow: public StageNode {
public:
    S_DEFINE_STAGE_NODE_META(STAGE_NODE_TYPE_SMOOTH_FOLLOW, "smooth_follow");
    S_DEFINE_STAGE_NODE_PARAM(SmoothFollow, "target", std::string,
                              std::string(),
                              "The name of the target to follow");

    SmoothFollow(Scene* owner);
    ~SmoothFollow();

    void on_late_update(float dt) override;

    void set_target(StageNodePtr node);
    bool has_target() const;
    StageNode* target() const;

    void set_follow_distance(float dist) {
        distance_ = dist;
    }
    void set_follow_height(float height) {
        height_ = height;
    }
    void set_damping(float damping);
    void set_rotation_damping(float damping);

    /* When set to false, the node will stay focused
     * on the target, but movement will stop */
    void set_following_enabled(bool v);

    const AABB& aabb() const override {
        return AABB::ZERO;
    }

private:
    StageNode* target_ = nullptr;

    bool on_create(Params params) override;

    bool following_enabled_ = true;

    float distance_ = 10.0;
    float height_ = 10.0;

    float damping_ = 5.0;
    float rotation_damping_ = 10.0;

    sig::Connection destroy_conn_;
};

} // namespace smlt
