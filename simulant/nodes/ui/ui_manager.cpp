
#include "ui_manager.h"
#include "button.h"
#include "label.h"
#include "progress_bar.h"
#include "../../stage.h"

namespace smlt {
namespace ui {

UIManager::UIManager(Stage *stage):
    stage_(stage) {

    manager_.reset(new WidgetManager());
}

WidgetID UIManager::new_widget_as_button(const unicode &text, float width, float height) {
    auto button = manager_->make_as<Button>(this, &config_).fetch();
    button->set_resize_mode(config_.button_resize_mode_);
    button->set_padding(
        config_.button_padding_.left,
        config_.button_padding_.right,
        config_.button_padding_.bottom,
        config_.button_padding_.top
    );
    button->set_text(text);
    button->set_background_colour(config_.button_background_color_);
    button->set_foreground_colour(config_.button_foreground_color_);
    button->resize(
        (width == .0f) ? config_.button_width_ : width,
        (height == .0f) ? config_.button_height_ : height
    );

    stage_->add_child(button);

    return button->id();
}

WidgetID UIManager::new_widget_as_label(const unicode &text, float width, float height) {
    auto label = manager_->make_as<Label>(this, &config_).fetch();
    label->set_text(text);
    label->resize(width, height);

    stage_->add_child(label);

    return label->id();
}

WidgetID UIManager::new_widget_as_progress_bar(float min, float max, float value) {
    auto pg = manager_->make_as<ProgressBar>(this, &config_).fetch();
    pg->set_property("min", min);
    pg->set_property("max", max);
    pg->set_property("value", value);

    stage_->add_child(pg);

    return pg->id();
}

void UIManager::delete_widget(WidgetID widget) {
    manager_->destroy(widget);
}

}
}
