
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
}

std::size_t Rig::joint_count() const {
    return joints_.size();
}

void Rig::recalc_absolute_transformations() {
    for(std::size_t i = 0; i < joint_count(); ++i) {
        RigJoint* joint = &joints_[i];
        RigJoint* parent = joint->parent();
        if(!parent) {
            joint->absolute_rotation_ = joint->rotation() * joint->rotation_;
            joint->absolute_translation_ = joint->translation() + joint->translation_;
        } else {
            auto& parent_rot = parent->absolute_rotation_;
            joint->absolute_rotation_ = (
                parent_rot * joint->rotation() * joint->rotation_
            );

            joint->absolute_translation_ = (
                parent->absolute_translation_ +
                parent_rot.rotate_vector(joint->translation() + joint->translation_)
            );
        }
    }
}

RigJoint* Rig::joint(std::size_t index) {
    return &joints_[index];
}

void RigJoint::rotate_to(const Quaternion& rotation) {
    rotation_ = rotation;
}

void RigJoint::move_to(const Vec3& translation) {
    translation_ = translation;
}

}
