#pragma once

#include <memory>
#include <simulant/simulant.h>

namespace smlt {

struct SphereJointImpl;

class ReactiveBody;

class SphereJoint {
public:
    SphereJoint(
        ReactiveBody* a, ReactiveBody* b,
        const Vec3& aoff, const Vec3& boff
    );

    ~SphereJoint();

    ReactiveBody* first_body() const;
    ReactiveBody* second_body() const;

private:
    std::unique_ptr<SphereJointImpl> impl_;

    // FIXME: Poor attempt to hide implementation
    // we should probably have a private.h and move
    // SphereJointImpl there so that the PhysicsService
    // can see the definition
    friend class PhysicsService;
    void set_b3_joint(void* joint);
    void* get_b3_joint() const;
};

}
