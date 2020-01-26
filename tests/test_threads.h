#pragma once

#include "simulant/simulant.h"
#include "simulant/test.h"

#include "simulant/threads/future.h"

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

        auto except = []() -> int {
            throw std::runtime_error("Error!");
        };

        auto promise = async(func_argless);

        thread::sleep(100);

        assert_true(promise.is_ready());
        assert_equal(1, promise.get());

        promise = async(func, 1);

        thread::sleep(100);

        assert_true(promise.is_ready());
        assert_equal(2, promise.get());

        promise = async(except);
        thread::sleep(100);
        assert_true(promise.is_ready());
        assert_true(promise.is_failed());
    }
};

}
