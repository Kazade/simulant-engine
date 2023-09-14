#pragma once

#include "simulant/simulant.h"
#include "simulant/test.h"

namespace {

using namespace smlt;

class MathTest : public smlt::test::TestCase {
public:

    void test_lerp_angle() {
        smlt::Degrees a(10);
        smlt::Degrees b(270);

        auto ret = smlt::lerp_angle(a, b, 0.5f);
        assert_close(ret.value, -40, 0.0001f);

        ret = smlt::lerp_angle(a, b, 0.0f);
        assert_close(ret.value, a.value, 0.0001f);

        ret = smlt::lerp_angle(a, b, 1.0f);
        assert_true(ret.is_effectively_equal_to(b, 0.0001f));

        ret = smlt::lerp_angle(b, a, 0.5f);
        assert_true(ret.is_effectively_equal_to(smlt::Degrees(320.0f), 0.0001f));
    }

    void test_lerp() {
        float a = 100;
        float b = -100;

        assert_equal(0, smlt::lerp(a, b, 0.5f));
        assert_equal(50.0f, smlt::lerp(b, a, 0.75f));
        assert_close(120.0f, smlt::lerp(b, a, 1.1f), 0.0001f);
        assert_close(-120.0f, smlt::lerp(a, b, 1.1f), 0.0001f);
    }

    void test_fast_divide() {
        assert_close(1.0f / 100.0f, fast_divide(1.0f, 100.0f), 0.00001f);
        assert_close(1000.0f / 2.5f, fast_divide(1000.0f, 2.5f), 0.00001f);
    }
};

}
