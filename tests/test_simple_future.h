#pragma once


#include "simulant/generic/simple_future.h"

namespace {

/*
 * Because of limitations of atomic operations on the Dreamcast std::future
 * from GCC doesn't work, so on this platform we use a custom future-like construct
 * which just uses std::thread and std::mutex to give similar behaviour. Not
 * all operations are supported, only the ones used in Simulant.
 */
class SimpleFutureTests : public smlt::test::SimulantTestCase {
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

        std::atomic<bool> running;
        running = true;

        my_future = stdX::async([&running]() -> bool {
            while(running) {}
            return true;
        });

        assert_true(my_future.valid());
        running = false;
        my_future.get();

        assert_false(my_future.valid());
    }
};


}
