#pragma once

#include "simulant/simulant.h"
#include "simulant/test.h"


namespace {

using namespace smlt;

class WidgetTest : public smlt::test::SimulantTestCase {
public:
    void set_up() {
        SimulantTestCase::set_up();
        stage_ = scene->create_node<smlt::Stage>();
    }

    void tear_down() {
        stage_->destroy();
        SimulantTestCase::tear_down();
    }

    void test_set_text_with_newline() {
        auto label = scene->create_node<ui::Label>("This is\na\nnew\nline");
        auto label2 = scene->create_node<ui::Label>("This is");

        assert_equal(label->content_width(), label2->content_width());
        assert_true(label->height() > label2->height());
    }

    void test_materials_freed() {
        scene->create_node<ui::Label>("Seed the materials");

        auto mc = scene->assets->material_count();

        auto label = scene->create_node<ui::Label>("This is\na\nnew\nline");
        label->set_background_image(scene->assets->new_texture(16, 16));
        label->set_foreground_image(scene->assets->new_texture(16, 16));
        assert_equal(scene->assets->material_count(), mc + 2);  /* background, foreground */

        label->destroy();
        application->run_frame();

        assert_equal(scene->assets->material_count(), mc); /* Destroyed */
    }

    void test_foreground_and_background_images_differ() {
        auto button = scene->create_node<ui::Button>("Button", ui::Px(100), ui::Px(20));

        auto t1 = scene->assets->new_texture(8, 8, smlt::TEXTURE_FORMAT_RGBA_4UB_8888);
        auto t2 = scene->assets->new_texture(8, 8, smlt::TEXTURE_FORMAT_RGBA_4UB_8888);

        button->set_foreground_image(t1);
        button->set_background_image(t2);

        assert_equal(button->foreground_material()->diffuse_map(), t1);
        assert_equal(button->background_material()->diffuse_map(), t2);
    }

    void test_render_priority() {
        auto button = scene->create_node<ui::Button>("Button", ui::Px(100), ui::Px(20));
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

        auto button = scene->create_node<ui::Button>("Test", ui::Px(100), ui::Px(20));
        button->set_padding(0);
        button->set_border_width(0);

        assert_equal(button->aabb().min().x, 0); // By default, the bounds should start at zero
        assert_equal(button->aabb().min().y, 0);
        button->set_anchor_point(1.0, 0.0); // Bottom-right

        assert_equal(button->aabb().min().x, 0); // No change
        assert_equal(button->aabb().min().y, 0);
        button->transform->set_translation_2d(Vec2(0, 0));
        assert_equal(button->aabb().min().x, -button->aabb().width()); // Should've changed now
        assert_equal(button->aabb().min().y, 0);
    }

    void test_button_creation() {
        auto button = scene->create_node<ui::Button>("Test", ui::Px(100), ui::Px(20));

        assert_equal(_u("Test"), button->text());
        assert_equal(ui::Px(100), button->requested_width());
        assert_equal(ui::Px(20), button->requested_height());
    }

    void test_focus_chain() {
        auto widget1 = scene->create_node<ui::Label>("label1");
        auto widget2 = scene->create_node<ui::Label>("label2");

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

class TextEntryTests : public smlt::test::SimulantTestCase {
public:
    void set_up() {
        SimulantTestCase::set_up();

        stage_ = scene->create_node<smlt::Stage>();
    }

    void tear_down() {
        stage_->destroy();
        SimulantTestCase::tear_down();
    }

    void test_set_text() {
        auto entry = scene->create_node<ui::TextEntry>("Hello");
        assert_equal(entry->text(), "Hello");
    }

private:
    StagePtr stage_;
};

class ProgressBarTests : public smlt::test::SimulantTestCase {
public:
    void set_up() {
        SimulantTestCase::set_up();

        stage_ = scene->create_node<smlt::Stage>();
    }

    void tear_down() {
        stage_->destroy();
        SimulantTestCase::tear_down();
    }

    void test_set_value() {
        auto progress_bar = scene->create_node<ui::ProgressBar>(0, 100, 50);

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
        stage_ = scene->create_node<smlt::Stage>();
    }

    void tear_down() {
        stage_->destroy();
        SimulantTestCase::tear_down();
    }

    void test_image_creation() {
        auto texture = scene->assets->new_texture_from_file("simulant-icon.png");
        auto image = scene->create_node<ui::Image>(texture);

        assert_equal(image->width(), texture->width());
        assert_equal(image->height(), texture->height());
        assert_true(image->has_background_image());
        assert_false(image->has_foreground_image());
        assert_equal(image->resize_mode(), smlt::ui::RESIZE_MODE_FIXED);
    }

    void test_set_source_rect() {
        auto texture = scene->assets->new_texture_from_file("simulant-icon.png");
        auto image = scene->create_node<ui::Image>(texture);

        image->set_source_rect(smlt::ui::UICoord(ui::Px(), ui::Px()), smlt::ui::UICoord(ui::Px(128), ui::Px(128)));

        assert_equal(image->width(), 128);
        assert_equal(image->height(), 128);
    }

private:
    StagePtr stage_;

};

class FrameTests : public smlt::test::SimulantTestCase {
public:
    void set_up() {
        SimulantTestCase::set_up();

        stage_ = scene->create_node<smlt::Stage>();
    }

    void tear_down() {
        stage_->destroy();
        SimulantTestCase::tear_down();
    }

    void test_add_child() {
        _setup_frame();
    }

    void test_remove_child() {
        smlt::ui::Frame* frame = _setup_frame();

        auto first_child = frame->packed_children().front();
        assert_equal(frame->packed_children().size(), 2u);
        frame->unpack_child(first_child);
        assert_equal(frame->packed_children().size(), 1u);

        auto p = frame->padding();
        auto b = frame->border_width();

        first_child = frame->packed_children().front();
        assert_equal(
            frame->outer_height(),
            first_child->outer_height() + p.top + p.bottom + (b * 2)
        );
    }

    void test_frame_set_layout_direction() {
        smlt::ui::Frame* frame = _setup_frame();
        frame->set_layout_direction(smlt::ui::LAYOUT_DIRECTION_LEFT_TO_RIGHT);

        auto p = frame->padding();
        auto b = frame->border_width();

        smlt::ui::Px expected_width = p.left + p.right + (b * 2);
        for(auto& child: frame->packed_children()) {
            expected_width += child->outer_width();
        }

        expected_width += (frame->space_between() * int(frame->packed_children().size() - 1));

        assert_equal(frame->outer_width(), expected_width);
    }

    void test_widgets_are_reparented() {
        smlt::ui::Frame* frame = _setup_frame();
        auto& children = frame->packed_children();

        for(auto& child: children) {
            assert_equal(child->parent(), frame);
        }

        auto child1 = children[0];
        frame->unpack_child(child1, smlt::ui::CHILD_CLEANUP_RETAIN);

        /* Child parent should be the stage */
        assert_true(child1->parent_is_scene());
    }

private:
    smlt::ui::Frame* _setup_frame() {
        smlt::ui::Frame* frame = scene->create_node<ui::Frame>("");
        smlt::ui::Button* button = scene->create_node<ui::Button>("Button 1");
        smlt::ui::Label* label = scene->create_node<ui::Label>("Test Label");

        /* Can pack a child once, but not itself */
        assert_true(frame->pack_child(button));
        assert_false(frame->pack_child(button));
        assert_false(frame->pack_child(frame));

        /* By default, the frame would resize to contain the button */
        auto p = frame->padding();
        auto b = frame->border_width();

        auto fw = frame->outer_width();
        auto fh = frame->outer_height();
        auto bw = button->outer_width();
        auto bh = button->outer_height();
        assert_equal(fw, bw + p.left + p.right + (b * 2));
        assert_equal(fh, bh + p.top + p.bottom + (b * 2));

        assert_true(frame->pack_child(label));

        auto max_width = std::max(button->outer_width(), label->outer_width());
        auto child_height = button->outer_height() + label->outer_height();
        auto spacing = frame->space_between();

        fw = frame->outer_width();
        fh = frame->outer_height();

        assert_equal(fw, max_width + p.left + p.right + (b * 2));
        assert_equal(fh, child_height + spacing + p.top + p.bottom + (b * 2));
        return frame;
    }

    StagePtr stage_;
};


}
