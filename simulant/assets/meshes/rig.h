#pragma once

#include <string>

#include "../../math/quaternion.h"
#include "../../math/vec3.h"
#include "../../generic/containers/heap_array.h"

namespace smlt {

class Rig;
class Skeleton;

class RigJoint {
public:
    RigJoint() {}

    void rotate_to(const smlt::Quaternion& rotation);
    void move_to(const smlt::Vec3& translation);

    bool overrides_rotation() const {
        return active_state_ & ROTATION_ACTIVE;
    }

    bool overrides_translation() const {
        return active_state_ & TRANSLATION_ACTIVE;
    }

    void reset_rotation();
    void reset_translation();

    void reset() {
        reset_rotation();
        reset_translation();
    }

private:
    enum ActiveState {
        TRANSLATION_ACTIVE = 1,
        ROTATION_ACTIVE = 2
    };

    friend class Rig;

    Rig* rig_ = nullptr;

    Vec3 translation_;
    Quaternion rotation_;
    uint8_t active_state_ = 0;
};


class Rig {
public:
    Rig(const Skeleton* skeleton);
    RigJoint* joint(std::size_t index);

    /* Should always be equal to the skeleton joint count */
    std::size_t joint_count() const;

    /* Returns true if this Rig is overriding the associated
     * skeleton */
    bool is_active() const;
private:
    friend class RigJoint;

    heap_array<RigJoint> joints_;
    std::size_t joints_overridden_count_ = 0;
};

};
