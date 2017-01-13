
#include "ui_manager.h"
#include "button.h"
#include "label.h"
#include "progress_bar.h"

namespace smlt {
namespace ui {

UIManager::UIManager(Stage *stage):
    stage_(stage) {

}

WidgetID UIManager::new_widget_as_button(const unicode &text, float width, float height) {
    auto button = manager_->make_as<Button>(this, &config_).fetch();
    button->set_text(text);
    button->resize(width, height);
    return button->id();
}

WidgetID UIManager::new_widget_as_label(const unicode &text, float width, float height) {
    auto label = manager_->make_as<Label>(this, &config_).fetch();
    label->set_text(text);
    label->resize(width, height);
    return label->id();
}

WidgetID UIManager::new_widget_as_progress_bar(float min, float max, float value) {
    auto pg = manager_->make_as<ProgressBar>(this, &config_).fetch();
    pg->set_property("min", min);
    pg->set_property("max", max);
    pg->set_property("value", value);

    return pg->id();
}

void UIManager::delete_widget(WidgetID widget) {
    manager_->destroy(widget);
}

}
}
