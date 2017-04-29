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

    auto wanted_position = target->absolute_position() + Vec3(0, height_, distance_).rotated_by(target->absolute_rotation());

    owner->move_to_absolute(
        owner->absolute_position().lerp(wanted_position, damping_ * dt)
    );

    auto wanted_rotation = Quaternion::as_look_at((target->absolute_position() - owner->absolute_position()).normalized(), target->absolute_rotation().up());
    wanted_rotation.inverse(); // << FIXME: This seems like a bug in as_look_at...

    owner->rotate_to_absolute(
        owner->absolute_rotation().slerp(wanted_rotation, rotation_damping_ * dt)
    );
}

void SmoothFollow::set_target(ActorPtr actor) {
    target_ = actor->shared_from_this();
}

void SmoothFollow::set_target(ParticleSystemPtr ps) {
    target_ = ps->shared_from_this();
}

}
}
