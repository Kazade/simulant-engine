#include <simulant/simulant.h>
#include "bounce/bounce.h"

#include "joints.h"

namespace smlt {

struct SphereJointImpl {
    b3Joint* joint_ = nullptr;
    ReactiveBody* body_a_ = nullptr;
    ReactiveBody* body_b_ = nullptr;
};

SphereJoint::SphereJoint(
    ReactiveBody* a, ReactiveBody* b,
    const Vec3& aoff, const Vec3& boff
):
    impl_(new SphereJointImpl()) {

    auto sim = a->scene->find_service<PhysicsService>();
    if(sim) {
        sim->init_sphere_joint(this, a, b, aoff, boff);
    }

    impl_->body_a_ = a;
    impl_->body_b_ = b;
}

SphereJoint::~SphereJoint() {
    auto sim = impl_->body_a_->scene->find_service<PhysicsService>();
    if(sim) {
        sim->release_sphere_joint(this);
    }
}

ReactiveBody *SphereJoint::first_body() const {
    return impl_->body_a_;
}

ReactiveBody *SphereJoint::second_body() const {
    return impl_->body_b_;
}

void SphereJoint::set_b3_joint(void *joint) {
    impl_->joint_ = (b3Joint*) joint;
}

void* SphereJoint::get_b3_joint() const {
    return impl_->joint_;
}

}
