#pragma once

#include "simulant/simulant.h"
#include "simulant/utils/half.hpp"

namespace {

using namespace smlt;

class HalfFloatTests: public smlt::test::SimulantTestCase {
public:
    void test_basic_usage() {
        half one = 1.0f;
        half hundred = 100.0f;

        half sum = one + hundred;
        assert_close(float(sum), 101.0f, 0.01f);

        half product = one * hundred;
        assert_close(float(product), 100.0f, 0.01f);

        half quotient = hundred / one;
        assert_close(float(quotient), 100.0f, 0.01f);

        half difference = hundred - one;
        assert_close(float(difference), 99.0f, 0.01f);

        sum += one;
        assert_close(float(sum), 102.0f, 0.01f);

        product *= one;
        assert_close(float(product), 100.0f, 0.01f);

        quotient /= one;
        assert_close(float(quotient), 100.0f, 0.01f);

        difference -= one;
        assert_close(float(difference), 98.0f, 0.01f);
    }

    void test_representation() {
        auto zero = half();
        assert_equal(*zero.data(), uint16_t(0x0000));

        // FIXME: this test fails, but probably shouldn't!
        // auto smallest = half(0.00006103515625f); // 2^-14
        // assert_equal(*smallest.data(), uint16_t(0x0001));

        auto largest = half(65504);
        assert_equal(*largest.data(), uint16_t(0x7BFF));

        auto pi = half(3.140625f);
        assert_equal(*pi.data(), uint16_t(0x4248));

        auto infinity = std::numeric_limits<float>::infinity();
        auto half_infinity = half(infinity);
        assert_equal(*half_infinity.data(), uint16_t(0x7C00));
    }
};

} // namespace
