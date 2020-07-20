#pragma once

#include <simulant/test.h>

namespace {

using namespace smlt;

class PanelTests : public test::SimulantTestCase {
public:
    void tear_down() {
        core->deactivate_panel(1);
    }

    void test_toggle_panel() {
        assert_false(core->panel_is_active(1));
        core->toggle_panel(1);
        assert_true(core->panel_is_active(1));
        core->toggle_panel(1);
        assert_false(core->panel_is_active(1));
    }
};

}
