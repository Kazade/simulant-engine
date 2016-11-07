#ifndef TEST_CURVE_H
#define TEST_CURVE_H

#include "kaztest/kaztest.h"

#include "kglt/extra/ai/curve.h"

class CurveTest : public SimulantTestCase {
public:
    void test_bezier() {
        kglt::Vec3 start_pos(-1, 0, 0);
        kglt::Vec3 start_dir(0, 1, 0);

        kglt::Vec3 end_pos(1, 0, 0);
        kglt::Vec3 end_dir(0, -1, 0);

        kglt::extra::Bezier curve(start_pos, start_dir, end_pos, end_dir);

        assert_equal(start_pos, curve.position(0.0));
        assert_equal(end_pos, curve.position(1.0));
    }

};


#endif // TEST_CURVE_H
