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

    void test_default_promise() {
        Promise<bool> ret;

        assert_false(ret.is_initialized());

        ret = cr_async([]() -> bool {
            return true;
        });

        assert_true(ret.is_initialized());
        cr_await(ret);

        assert_true(ret.is_ready());
        assert_true(ret.value());
    }

    void test_then() {
        bool called = false;
        auto callback = [&](bool value) -> bool {
            called = value;
            return called;
        };

        auto ret = cr_async([]() -> bool {
            return true;
        }).then(callback);

        cr_await(ret);
        assert_true(called);
    }

    void test_yield_and_wait() {
        bool called = false;

        auto ret = cr_async([&]() -> bool {
            cr_yield_for(Seconds(0.1f));
            called = true;
            return true;
        });

        application->update_coroutines();
        assert_false(called);
        thread::sleep(200);
        application->update_coroutines();
        assert_true(called);
    }

    void test_coroutine_order() {
        /* All coroutines should run after each update */
        int counter = 3;
        std::vector<int> order;
        bool done = false;

        std::function<void (int)> cb = [&](int a) {
            /* Trigger a cr from a cr. The new cr should
             * go to the end, and still be run in order */
            if(a < 3) {
                cr_async([&]() { cb(a + 1); });
            }

            while(!done) {
                order.push_back(a);
                ++counter;
                cr_yield();
            }
        };

        auto update_cb = [&](float) {
            assert_equal(counter, 3);
            counter = 0;
        };

        cr_async([&]() { cb(1); });

        auto sig = application->signal_update().connect(update_cb);
        application->run_frame();
        application->run_frame();
        application->run_frame();

        assert_equal(order.size(), 9u);
        assert_equal(order[0], 1);
        assert_equal(order[1], 2);
        assert_equal(order[2], 3);
        assert_equal(order[3], 1);
        assert_equal(order[4], 2);
        assert_equal(order[5], 3);
        assert_equal(order[6], 1);
        assert_equal(order[7], 2);
        assert_equal(order[8], 3);

        /* Make sure everything quits nicely */
        done = true;
        application->stop_all_coroutines();
        sig.disconnect();
    }
};

}
