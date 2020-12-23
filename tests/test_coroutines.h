#pragma once

#include "simulant/test.h"

namespace {

using namespace smlt;


class CoroutineTests : public test::SimulantTestCase {
public:
    void test_await() {
        auto value = cr_await(
            cr_async([]() -> int {
                int j = 0;
                for(int i = 0; i < 100; ++i) {
                    j++;
                    cr_yield();
                }

                return j;
            })
        );

        assert_equal(value, 100);
    }

};

}
