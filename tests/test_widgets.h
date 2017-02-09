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
        auto button = stage_->ui->new_widget_as_button("Test", 100, 20).fetch();

        assert_equal(_u("Test"), button->text());
        assert_equal(100, button->requested_width());
        assert_equal(20, button->requested_height());
    }

    void test_focus_chain() {
        auto widget1 = stage_->ui->new_widget_as_label("label1").fetch();
        auto widget2 = stage_->ui->new_widget_as_label("label2").fetch();

        assert_is_null(widget1->focused_in_chain());

        widget1->set_focus_next(widget2);
        widget1->focus();

        assert_equal(widget1, widget1->focused_in_chain());
        widget1->focus_next_in_chain();

        assert_equal(widget2, widget2->focused_in_chain());

        widget2->blur();

        assert_is_null(widget1->focused_in_chain());
    }

private:
    StagePtr stage_;
};

}
