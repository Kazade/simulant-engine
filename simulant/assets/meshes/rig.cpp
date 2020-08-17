
#include "rig.h"
#include "skeleton.h"

namespace smlt {

Rig::Rig(const Skeleton* skeleton):
    joints_(skeleton->joint_count()) {

    for(std::size_t i = 0; i < joints_.size(); ++i) {
        joints_[i].rig_ = this;
    }
}

std::size_t Rig::joint_count() const {
    return joints_.size();
}

bool Rig::is_active() const {
    return joints_overridden_count_ > 0;
}

RigJoint* Rig::joint(std::size_t index) {
    return &joints_[index];
}

void RigJoint::rotate_to(const Quaternion& rotation) {
    if(!active_state_) {
        rig_->joints_overridden_count_++;
    }

    active_state_ |= ROTATION_ACTIVE;
    rotation_ = rotation;
}

void RigJoint::move_to(const Vec3& translation) {
    if(!active_state_) {
        rig_->joints_overridden_count_++;
    }

    active_state_ |= TRANSLATION_ACTIVE;
    translation_ = translation;
}

void RigJoint::reset_rotation() {
    active_state_  &= ~ROTATION_ACTIVE;
    if(!active_state_) {
        rig_->joints_overridden_count_--;
    }
}

void RigJoint::reset_translation() {
    active_state_  &= ~TRANSLATION_ACTIVE;
    if(!active_state_) {
        rig_->joints_overridden_count_--;
    }
}

}
