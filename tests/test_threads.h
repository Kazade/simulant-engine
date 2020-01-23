#pragma once

#include "simulant/simulant.h"
#include "simulant/test.h"

#include "simulant/threads/async.h"

namespace {

using namespace smlt;
using namespace smlt::thread;

class ThreadTests : public smlt::test::SimulantTestCase {
public:
    void test_async() {
        auto func_argless = []() -> int {
            return 1;
        };

        auto func = [](int arg) -> int {
            return arg + 1;
        };

        auto promise = async(func_argless);

        window->platform->sleep_ms(500);

        assert_true(promise.is_ready());
        assert_equal(1, promise.value());

        promise = async(func, 1);

        window->platform->sleep_ms(500);

        assert_true(promise.is_ready());
        assert_equal(2, promise.value());
    }
};

}
