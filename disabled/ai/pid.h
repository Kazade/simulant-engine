#ifndef VEC3_PID_CONTROLLER_H
#define VEC3_PID_CONTROLLER_H

#include "../../generic/managed.h"
#include "../../types.h"

namespace kglt {
namespace extra {

template<typename T>
class PIDController : public Managed<PIDController<T> > {
public:
    PIDController(float p, float i, float d):
        p_(p),
        i_(i),
        d_(d) {}

    T update(const T& current_error, double time_frame) {
        integral_ += current_error * time_frame;
        auto deriv = (current_error - last_error_) / time_frame;
        last_error_ = current_error;
        return (current_error * p_) + (integral_ * i_) + (deriv * d_);
    }

private:
    float p_;
    float i_;
    float d_;

    T integral_;
    T last_error_;
};

typedef PIDController<kglt::Vec3> Vec3PIDController;
typedef PIDController<float> FloatPIDController;
typedef PIDController<double> DoublePIDController;

}
}

#endif // VEC3_PID_CONTROLLER_H
