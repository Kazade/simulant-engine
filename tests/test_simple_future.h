#pragma once

#include "global.h"
#include "simulant/generic/simple_future.h"

namespace {

/*
 * Because of limitations of atomic operations on the Dreamcast std::future
 * from GCC doesn't work, so on this platform we use a custom future-like construct
 * which just uses std::thread and std::mutex to give similar behaviour. Not
 * all operations are supported, only the ones used in Simulant.
 */
class SimpleFutureTests : public SimulantTestCase {
public:
    void test_wait() {

    }

    void test_wait_for() {

    }

    void test_get() {
        auto future = stdX::async([]() -> bool { return true; });
        assert_true(future.get());
    }

    void test_valid() {
        stdX::future<bool> my_future;
        assert_false(my_future.valid());

        my_future = stdX::async([]() -> bool { return true; });

        assert_true(my_future.valid());
    }
};


}
