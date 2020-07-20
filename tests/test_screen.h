#pragma once

#include "simulant/test.h"
#include "simulant/macros.h"

namespace {

using namespace smlt;

class ScreenTests : public smlt::test::SimulantTestCase {

public:
    void test_registering() {
        uint8_t added = 0;

        auto conn = core->signal_screen_added().connect([&](std::string name, Screen* screen) {
            _S_UNUSED(name);
            _S_UNUSED(screen);
            added++;
        });

        auto screen = core->_create_screen("Test1", 32, 32, SCREEN_FORMAT_G1, 60);

        assert_is_not_null(screen);
        assert_equal(added, 1);
        assert_equal(core->screen_count(), 1u);
        assert_equal(core->screen("Test1"), screen);

        conn.disconnect();

        core->_destroy_screen("Test1");

        assert_equal(core->screen_count(), 0u);
        assert_is_null(core->screen("Test1"));
    }
};


}
