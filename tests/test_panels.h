#pragma once

#include <simulant/test.h>

namespace {

using namespace smlt;

class PanelTests : public test::SimulantTestCase {
public:
    void tear_down() {
        window->deactivate_panel(1);
    }

    void test_toggle_panel() {
        assert_false(window->panel_is_active(1));
        window->toggle_panel(1);
        assert_true(window->panel_is_active(1));
        window->toggle_panel(1);
        assert_false(window->panel_is_active(1));
    }
};

}
