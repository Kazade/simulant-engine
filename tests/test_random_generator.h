#pragma once


#include "../simulant/utils/random.h"

namespace {

using namespace smlt;

class RandomGeneratorTest : public smlt::test::SimulantTestCase {
public:
    void test_float_in_range() {
        RandomGenerator gen;

        for(uint32_t i = 0; i < 5; ++i) {
            auto v = gen.float_in_range(0.0f, 5.0f);
            assert_true(v >= 0.0f && v <= 5.0f);
        }
    }

    void test_int_in_range() {
        RandomGenerator gen;

        for(uint32_t i = 0; i < 5; ++i) {
            auto v = gen.int_in_range(-5, 5);
            assert_true(v >= -5 && v <= 5);
        }
    }
};

}
