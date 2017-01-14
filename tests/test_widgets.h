#pragma once

#include <kaztest/kaztest.h>
#include "../simulant/simulant.h"
#include "global.h"

namespace {

using namespace smlt;

class WidgetTest : public SimulantTestCase {
public:
    void set_up() {
        SimulantTestCase::set_up();
        stage_ = window->new_stage().fetch();
    }

    void tear_down() {
        window->delete_stage(stage_->id());
        SimulantTestCase::tear_down();
    }

    void test_button_creation() {
        auto button = stage_->ui->new_widget_as_button("Test", 100, 20);

        assert_equal("Test", button->text());
        assert_equal(100, button->requested_width());
        assert_equal(20, button->requested_height());
    }

private:
    StagePtr stage_;
};

}
