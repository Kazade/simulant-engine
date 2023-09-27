#include "smooth_follow.h"

namespace smlt {

SmoothFollow::SmoothFollow(Scene* owner):
    StageNode(owner, STAGE_NODE_TYPE_SMOOTH_FOLLOW) {

}

SmoothFollow::~SmoothFollow() {
    if(destroy_conn_) {
        destroy_conn_.disconnect();
    }
}

void SmoothFollow::on_late_update(float dt) {
    auto target = target_;

    if(!target) {
        return;
    }

    auto target_rotation = target->absolute_rotation();
    auto target_position = target->absolute_position();
    auto dir = target_position - absolute_position();

    /* We only move the camera if following is enabled, otherwise we
     * just change the rotation */
    if(following_enabled_) {
        assert(!(std::isnan(target_position.x) || std::isnan(target_position.y) || std::isnan(target_position.z)));

        auto wanted_position = target_position + Vec3(0, height_, distance_).rotated_by(target_rotation);

        // Keep within 0.0 - 1.0f;
        auto damping_to_apply = std::max(std::min(damping_ * dt, 1.0f), 0.0f);
        move_to_absolute(
            absolute_position().lerp(wanted_position, damping_to_apply)
        );

        if(smlt::almost_equal(dir.length_squared(), 0.0f)) {
            // The two things are the same, we can't really do much about rotation so just
            // return
            return;
        }
    }

    auto wanted_rotation = Quaternion::look_rotation(dir.normalized(), target_rotation.up());

    // Keep within 0.0 - 1.0f;
    auto rot_damping_to_apply = std::max(std::min(rotation_damping_ * dt, 1.0f), 0.0f);
    rotate_to_absolute(
        absolute_rotation().slerp(wanted_rotation, rot_damping_to_apply)
    );
}

void SmoothFollow::set_target(StageNodePtr node) {
    if(destroy_conn_) {
        destroy_conn_.disconnect();
    }

    target_ = node;

    if(target_) {
        destroy_conn_ = target_->signal_destroyed().connect([&]() {
            set_target(nullptr);
        });
    }
}

void SmoothFollow::set_damping(float damping) {
    damping_ = damping;
}

void SmoothFollow::set_rotation_damping(float damping) {
    rotation_damping_ = damping;
}

void SmoothFollow::set_following_enabled(bool v) {
    following_enabled_ = v;
}

bool SmoothFollow::on_create(void* params) {
    SmoothFollowParams* args = (SmoothFollowParams*) params;
    set_target(args->target);
    return true;
}

bool SmoothFollow::has_target() const {
    return bool(target_);
}

StageNode* SmoothFollow::target() const {
    return target_;
}

}
