#ifndef CURVE_H
#define CURVE_H

#include <kazbase/exceptions.h>

#include "../../generic/managed.h"
#include "../../types.h"

namespace kglt {
namespace extra {

/*
 *  Represents a bezier curve. Provides a single function that
 *  returns the position as a 3D vector when given an interpolation value
 *  of t
 */
class Bezier:
    public Managed<Bezier> {
public:
    Bezier(const kglt::Vec3& start_pos, const kglt::Vec3& start_dir,
           const kglt::Vec3& end_pos, const kglt::Vec3& end_dir):
        start_pos_(start_pos),
        start_dir_(start_dir),
        end_pos_(end_pos),
        end_dir_(end_dir) {}

    kglt::Vec3 position(double t) const {
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

    double length() const {
        /*
         * Approximates the length of a bezier curve by dividing it
         * into segments and combining the lengths
         */

        static const int STEPS = 10;

        double length = 0.0;

        kglt::Vec3 prev;
        for(int i = 0; i < STEPS; ++i) {
            double t = double(i) / double(STEPS);
            kglt::Vec3 p = position(t);

            if(i > 0) {
                length += (p - prev).length();
            }

            prev = p;
        }
        return length;
    }

    kglt::Vec3 control(int32_t idx) {
        assert(idx >= 0 && idx < 4);

        switch(idx) {
            case 0: return start_pos_;
            case 1: return start_pos_ + start_dir_;
            case 2: return end_pos_ + end_dir_;
            case 3: return end_pos_;
            default:
                throw LogicError("Invalid control point index");
        }
    }

private:
    kglt::Vec3 start_pos_;
    kglt::Vec3 start_dir_;
    kglt::Vec3 end_pos_;
    kglt::Vec3 end_dir_;
};

}
}

#endif // CURVE_H
