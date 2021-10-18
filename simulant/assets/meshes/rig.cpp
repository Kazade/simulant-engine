
#include <map>
#include "rig.h"
#include "skeleton.h"

namespace smlt {

Rig::Rig(const Skeleton* skeleton):
    joints_(skeleton->joint_count()) {

    std::map<const Joint*, std::size_t> parent_lookup;

    for(std::size_t i = 0; i < joints_.size(); ++i) {
        joints_[i].rig_ = this;
        joints_[i].skeleton_joint_ = skeleton->joint(i);

        /* Copy the parent relationship from the associated skeleton.
         * This is necessary so we can recalculate absolute positions */
        parent_lookup[joints_[i].skeleton_joint_] = i;

        auto p = joints_[i].skeleton_joint_->parent();
        if(p) {
            joints_[i].parent_ = &joints_[parent_lookup.at(p)];
        }
    }

    absolute_transformations_dirty_ = true;
}

std::size_t Rig::joint_count() const {
    return joints_.size();
}

void Rig::recalc_absolute_transformations() {
    if(!absolute_transformations_dirty_) {
        return;
    }

    for(std::size_t i = 0; i < joint_count(); ++i) {
        RigJoint* joint = &joints_[i];
        RigJoint* parent = joint->parent();
        const Joint* skj = joint->skeleton_joint_;

        if(!parent) {
            joint->absolute_rotation_ = skj->rotation() * joint->rotation_;
            joint->absolute_translation_ = skj->translation() + joint->translation_;
        } else {
            auto& parent_rot = parent->absolute_rotation_;
            joint->absolute_rotation_ = (
                parent_rot * skj->rotation() * joint->rotation_
            );

            joint->absolute_translation_ = (
                parent->absolute_translation_ +
                parent_rot * (skj->translation() + joint->translation_)
            );
        }
    }

    absolute_transformations_dirty_ = false;
}

RigJoint* Rig::joint(std::size_t index) {
    return &joints_[index];
}

RigJoint* Rig::find_joint(const std::string &name) {
    for(auto i = 0u; i < joints_.size(); ++i) {
        if(joints_[i].skeleton_joint_->name() == name) {
            return &joints_[i];
        }
    }

    return nullptr;
}

void RigJoint::rotate_to(const Quaternion& rotation) {
    if(rotation_ == rotation) {
        return;
    }

    rotation_ = rotation;
    rig_->absolute_transformations_dirty_ = true;
}

void RigJoint::move_to(const Vec3& translation) {
    if(translation_ == translation) {
        return;
    }

    translation_ = translation;
    rig_->absolute_transformations_dirty_ = true;
}

std::string RigJoint::name() const {
    return skeleton_joint_->name();
}

}
