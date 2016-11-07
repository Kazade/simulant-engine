#ifndef TEST_VECTOR_H
#define TEST_VECTOR_H

#include "kaztest/kaztest.h"

#include "simulant/simulant.h"

class VectorTest : public TestCase {
public:
    void test_vec2_rotated_by() {
        smlt::Vec2 up(0, 1);

        assert_equal(smlt::Vec2(-1, 0), up.rotated_by(90));
        assert_equal(smlt::Vec2(1, 0), up.rotated_by(-90));
    }

    void test_vec2_scale() {
        smlt::Vec2 up(0, 1);

        assert_equal(smlt::Vec2(0, 10), up * 10);

        up *= 10;

        assert_equal(smlt::Vec2(0, 10), up);
    }
};

#endif // TEST_VECTOR_H
