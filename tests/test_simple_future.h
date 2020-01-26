#pragma once


#include "simulant/threads/future.h"

namespace {

using namespace smlt;
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
        auto future = thread::async([]() -> bool { return true; });
        assert_true(future.get());
    }

    void test_valid() {
        thread::Future<bool> my_future;
        assert_false(my_future.is_valid());

        std::atomic<bool> running;
        running = true;

        my_future = thread::async([&running]() -> bool {
            while(running) {}
            return true;
        });

        assert_true(my_future.is_valid());
        running = false;
        my_future.get();

        assert_false(my_future.is_valid());
    }
};


}
