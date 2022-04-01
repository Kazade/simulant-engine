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
        auto callback = [&](bool value) -> bool{
            called = value;
            return called;
        };

        auto ret = cr_async([]() -> bool {
            return true;
        }).then(callback);

        cr_await(ret);
        assert_true(called);
    }
};

}
