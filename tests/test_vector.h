#ifndef TEST_VECTOR_H
#define TEST_VECTOR_H

#include "kglt/kglt.h"
#include "kglt/extra/ai/pid.h"
#include "kglt/kazbase/testing.h"

class VectorTest : public TestCase {
public:
    void test_vec2_rotated_by() {
        kglt::Vec2 up(0, 1);

        assert_equal(kglt::Vec2(-1, 0), up.rotated_by(90));
        assert_equal(kglt::Vec2(1, 0), up.rotated_by(-90));
    }

    void test_vec2_scale() {
        kglt::Vec2 up(0, 1);

        assert_equal(kglt::Vec2(0, 10), up * 10);

        up *= 10;

        assert_equal(kglt::Vec2(0, 10), up);
    }

    void test_vec3_pid_controller() {
        auto controller = kglt::extra::Vec3PIDController::create(20, 0, 0.25);

        kglt::Vec3 current(1, 0, 0);
        kglt::Vec3 desired(-1, 0, 0);

        float diff = fabs(current.x - desired.x);
        for(int i = 0; i < 30; ++i) {
            current += controller->update(desired - current, 1.0 / 60).limit(0.1);

            float new_diff = fabs(current.x - desired.x);
            diff = new_diff;
        }

        assert_close(desired.x, current.x, 0.001);

    }
};

#endif // TEST_VECTOR_H
