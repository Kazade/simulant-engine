#pragma once

#include "simulant/simulant.h"
#include "simulant/test.h"


namespace {

using namespace smlt;

class WidgetTest : public smlt::test::SimulantTestCase {
public:
    void set_up() {
        SimulantTestCase::set_up();
        stage_ = window->new_stage();
    }

    void tear_down() {
        window->destroy_stage(stage_->id());
        SimulantTestCase::tear_down();
    }

    void test_set_text_with_newline() {
        auto label = stage_->ui->new_widget_as_label("This is\na\nnew\nline");
        auto label2 = stage_->ui->new_widget_as_label("This is");

        assert_equal(label->content_width(), label2->content_width());
        assert_true(label->height() > label2->height());
    }

    void test_foreground_and_background_images_differ() {
        auto button = stage_->ui->new_widget_as_button("Button", 100, 20);

        auto t1 = stage_->assets->new_texture(8, 8, smlt::TEXTURE_FORMAT_RGBA_4UB_8888);
        auto t2 = stage_->assets->new_texture(8, 8, smlt::TEXTURE_FORMAT_RGBA_4UB_8888);

        button->set_foreground_image(t1);
        button->set_background_image(t2);

        assert_equal(button->foreground_material()->diffuse_map(), t1);
        assert_equal(button->background_material()->diffuse_map(), t2);
    }

    void test_render_priority() {
        auto button = stage_->ui->new_widget_as_button("Button", 100, 20);
        assert_equal(button->render_priority(), RENDER_PRIORITY_MAIN);
        button->set_render_priority(RENDER_PRIORITY_NEAR);
        assert_equal(button->render_priority(), RENDER_PRIORITY_NEAR);
        auto child = dynamic_cast<Actor*>(button->first_child());
        assert_is_not_null(child);
        assert_equal(child->render_priority(), RENDER_PRIORITY_NEAR);
    }

    void test_anchor_point() {
        /*
         * The anchor point should allow choosing where the
         * position of widgets is set from, it should start
         * at the bottom left initially
         */

        auto button = stage_->ui->new_widget_as_button("Test", 100, 20);
        assert_equal(button->aabb().min().x, 0); // By default, the bounds should start at zero
        assert_equal(button->aabb().min().y, 0);
        button->set_anchor_point(1.0, 0.0); // Bottom-right

        assert_equal(button->aabb().min().x, 0); // No change
        assert_equal(button->aabb().min().y, 0);
        button->move_to(0, 0);
        assert_equal(button->aabb().min().x, -button->aabb().width()); // Should've changed now
        assert_equal(button->aabb().min().y, 0);
    }

    void test_button_creation() {
        auto button = stage_->ui->new_widget_as_button("Test", 100, 20);

        assert_equal(_u("Test"), button->text());
        assert_equal(100, button->requested_width());
        assert_equal(20, button->requested_height());
    }

    void test_focus_chain() {
        auto widget1 = stage_->ui->new_widget_as_label("label1");
        auto widget2 = stage_->ui->new_widget_as_label("label2");

        assert_is_null((ui::Widget*) widget1->focused_in_chain());

        widget1->set_focus_next(widget2);
        widget1->focus();

        assert_equal(widget1, widget1->focused_in_chain());
        widget1->focus_next_in_chain();

        assert_equal(widget2, widget2->focused_in_chain());

        widget2->blur();

        assert_is_null((ui::Widget*) widget1->focused_in_chain());
    }

private:
    StagePtr stage_;
};


class ProgressBarTests : public smlt::test::SimulantTestCase {
public:
    void set_up() {
        SimulantTestCase::set_up();

        stage_ = window->new_stage();
    }

    void tear_down() {
        window->destroy_stage(stage_->id());
        SimulantTestCase::tear_down();
    }

    void test_set_value() {
        auto progress_bar = stage_->ui->new_widget_as_progress_bar(0, 100, 50);

        assert_equal(progress_bar->value(), 50);
        assert_equal(progress_bar->min(), 0);
        assert_equal(progress_bar->max(), 100);

        progress_bar->set_value(75);

        assert_equal(progress_bar->value(), 75);

        progress_bar->set_fraction(0.25);

        assert_equal(progress_bar->value(), 25);
    }

private:
    StagePtr stage_;
};

class ImageTests : public smlt::test::SimulantTestCase {
public:
    void set_up() {
        SimulantTestCase::set_up();
        stage_ = window->new_stage();
    }

    void tear_down() {
        window->destroy_stage(stage_->id());
        SimulantTestCase::tear_down();
    }

    void test_image_creation() {
        auto texture = stage_->assets->new_texture_from_file("simulant-icon.png");
        auto image = stage_->ui->new_widget_as_image(texture);

        assert_equal(image->width(), texture->width());
        assert_equal(image->height(), texture->height());
        assert_true(image->has_background_image());
        assert_false(image->has_foreground_image());
        assert_equal(image->resize_mode(), smlt::ui::RESIZE_MODE_FIXED);
    }

    void test_set_source_rect() {
        auto texture = stage_->assets->new_texture_from_file("simulant-icon.png");
        auto image = stage_->ui->new_widget_as_image(texture);

        image->set_source_rect(smlt::ui::UICoord(0, 0), smlt::ui::UICoord(128, 128));

        assert_equal(image->width(), 128);
        assert_equal(image->height(), 128);
    }

private:
    StagePtr stage_;

};

}
