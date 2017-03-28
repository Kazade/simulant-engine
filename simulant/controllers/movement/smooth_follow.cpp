#include "smooth_follow.h"

#include "../../nodes/actor.h"
#include "../../nodes/particles.h"

namespace smlt {
namespace controllers {

SmoothFollow::SmoothFollow(Controllable* controllable):
    StageNodeController(controllable) {

}

void SmoothFollow::late_update(float dt) {
    auto target = target_.lock();

    if(!target) {
        return;
    }

    Degrees target_rotation_angle = target->absolute_rotation().to_euler().y;
    float target_height = target->absolute_position().y + height_;

    Degrees current_rotation_angle = owner->absolute_rotation().to_euler().y;
    float current_height = owner->absolute_position().y;

    current_rotation_angle = math::lerp_angle(current_rotation_angle, target_rotation_angle, rotation_damping_ * dt);
    current_height = math::lerp(current_height, target_height, height_damping_ * dt);

    auto new_rotation = Quaternion(Degrees(0), current_rotation_angle, Degrees(0));
    owner->rotate_to_absolute(new_rotation);

    auto new_position = target->absolute_position();
    new_position = new_position - (Vec3(0, 0, -1) * distance_).rotated_by(new_rotation);
    new_position.y = current_height;

    owner->move_to_absolute(new_position);
    owner->look_at(target->absolute_position());
}

void SmoothFollow::set_target(ActorPtr actor) {
    target_ = actor->shared_from_this();
}

void SmoothFollow::set_target(ParticleSystemPtr ps) {
    target_ = ps->shared_from_this();
}

}
}
