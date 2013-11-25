#ifndef VEC3_PID_CONTROLLER_H
#define VEC3_PID_CONTROLLER_H

#include "../../generic/managed.h"
#include "../../types.h"

namespace kglt {
namespace extra {

class Vec3PIDController : public Managed<Vec3PIDController> {
public:
    Vec3PIDController(float p, float i, float d):
        p_(p),
        i_(i),
        d_(d) {}

    kglt::Vec3 update(const kglt::Vec3& current_error, float time_frame) {
        integral_ += current_error * time_frame;
        auto deriv = (current_error - last_error_) / time_frame;
        last_error_ = current_error;
        return (current_error * p_) + (integral_ * i_) + (deriv * d_);
    }

private:
    float p_;
    float i_;
    float d_;

    kglt::Vec3 integral_;
    kglt::Vec3 last_error_;
};

}
}

#endif // VEC3_PID_CONTROLLER_H
