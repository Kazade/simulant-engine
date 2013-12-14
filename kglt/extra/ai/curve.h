#ifndef CURVE_H
#define CURVE_H

#include "../../types.h"

namespace kglt {
namespace extra {

/*
 *  Represents a bezier curve. Provides a single function that
 *  returns the position as a 3D vector when given an interpolation value
 *  of t
 */
class Bezier {
public:
    Bezier(const kglt::Vec3& start_pos, const kglt::Vec3& start_dir,
           const kglt::Vec3& end_pos, const kglt::Vec3& end_dir):
        start_pos_(start_pos),
        start_dir_(start_dir),
        end_pos_(end_pos),
        end_dir_(end_dir) {}

    void position(double t) {
        return kglt::Vec3(
            scalar_position(start_pos_.x, start_dir_.x, end_dir_.x, end_pos.x, t),
            scalar_position(start_pos_.y, start_dir_.y, end_dir_.y, end_pos.y, t),
            scalar_position(start_pos_.z, start_dir_.z, end_dir_.z, end_pos.z, t)
        );
    }

private:
    void scalar_position(double A,  // Start value
                         double B,  // First control value
                         double C,  // Second control value
                         double D,  // Ending value
                         double t) {

        double s = 1 - t;
        double AB = A*s + B*t;
        double BC = B*s + C*t;
        double CD = C*s + D*t;
        double ABC = BC*s + CD*t;
        double BCD = BC*s + CD*t;
        return ABC*s + BCD*t;
    }

    kglt::Vec3 start_pos_;
    kglt::Vec3 start_dir_;
    kglt::Vec3 end_pos_;
    kglt::Vec3 end_dir_;
};

}
}

#endif // CURVE_H
