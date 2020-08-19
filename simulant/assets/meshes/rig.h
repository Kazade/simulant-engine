#pragma once

#include <string>

#include "../../math/quaternion.h"
#include "../../math/vec3.h"
#include "../../generic/containers/heap_array.h"

namespace smlt {

class Rig;
class Skeleton;
class Joint;

class RigJoint {
public:
    RigJoint() {}

    void rotate_to(const smlt::Quaternion& rotation);
    void move_to(const smlt::Vec3& translation);

    const Vec3& translation() const {
        return translation_;
    }

    const Quaternion& rotation() const {
        return rotation_;
    }

    RigJoint* parent() const {
        return parent_;
    }

private:
    friend class Rig;
    friend class SkeletalFrameUnpacker;

    Rig* rig_ = nullptr;
    RigJoint* parent_ = nullptr;

    const Joint* skeleton_joint_ = nullptr;

    Vec3 translation_, absolute_translation_;
    Quaternion rotation_, absolute_rotation_;    
};


class Rig {
public:
    Rig(const Skeleton* skeleton);
    RigJoint* joint(std::size_t index);

    /* Should always be equal to the skeleton joint count */
    std::size_t joint_count() const;

private:
    friend class RigJoint;
    friend class SkeletalFrameUnpacker;

    void recalc_absolute_transformations();

    heap_array<RigJoint> joints_;
};

}
