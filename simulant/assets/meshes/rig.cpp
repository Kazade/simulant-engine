
#include "rig.h"
#include "skeleton.h"

namespace smlt {

Rig::Rig(const Skeleton* skeleton):
    joints_(skeleton->joint_count()) {

    for(auto& j: joints_) {
        j.rig_ = this;
    }
}

}
