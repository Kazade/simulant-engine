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

    kglt::Vec3 position(double t) {
        if(t > 1.0) t = 1.0;
        if(t < 0.0) t = 0.0;


        float u = 1 - t;
        float tt = t * t;
        float uu = u * u;
        float ttt = tt * t;
        float uuu = uu * u;

        kglt::Vec3 p = uuu * start_pos_;
        p += 3 * uu * t * (start_pos_ + start_dir_);
        p += 3 * u * tt * (end_pos_ + end_dir_);
        p += ttt * end_pos_;

        return p;
    }

private:
    double scalar_position(double A,  // Start value
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
