#include <cmath>
#include "smooth_follow.h"

#include "../../nodes/actor.h"
#include "../../nodes/particle_system.h"

namespace smlt {
namespace behaviours {

SmoothFollow::SmoothFollow():
    StageNodeBehaviour() {

}

void SmoothFollow::late_update(float dt) {
    auto target = target_;

    if(!target) {
        return;
    }

    auto target_position = target->absolute_position();

    assert(!(std::isnan(target_position.x) || std::isnan(target_position.y) || std::isnan(target_position.z)));

    auto wanted_position = target_position + Vec3(0, height_, distance_).rotated_by(target->absolute_rotation());

    // Keep within 0.0 - 1.0f;
    auto damping_to_apply = std::max(std::min(damping_ * dt, 1.0f), 0.0f);
    stage_node->move_to_absolute(
        stage_node->absolute_position().lerp(wanted_position, damping_to_apply)
    );

    auto dir = target_position - stage_node->absolute_position();
    if(!dir.length_squared()) {
        // The two things are the same, we can't really do much about rotation so just
        // return
        return;
    }

    auto wanted_rotation = Quaternion::look_rotation(dir.normalized(), target->absolute_rotation().up());

    // Keep within 0.0 - 1.0f;
    auto rot_damping_to_apply = std::max(std::min(rotation_damping_ * dt, 1.0f), 0.0f);
    stage_node->rotate_to_absolute(
        stage_node->absolute_rotation().slerp(wanted_rotation, rot_damping_to_apply)
    );
}

void SmoothFollow::set_target(ActorPtr actor) {
    target_ = actor;
}

void SmoothFollow::set_target(ParticleSystemPtr ps) {
    target_ = ps;
}

void SmoothFollow::set_damping(float damping) {
    damping_ = damping;
}

void SmoothFollow::set_rotation_damping(float damping) {
    rotation_damping_ = damping;
}

}
}
