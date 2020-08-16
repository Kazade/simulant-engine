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
    void translate_to(const smlt::Vec3& translation);

    void reset_rotation();
    void reset_translation();

    void reset() {
        reset_rotation();
        reset_translation();
    }

private:
    friend class Rig;

    const Rig* rig_ = nullptr;
};


class Rig {
public:
    Rig(const Skeleton* skeleton);
    RigJoint* joint(const std::string& name);

private:
    heap_array<RigJoint> joints_;
};

};
