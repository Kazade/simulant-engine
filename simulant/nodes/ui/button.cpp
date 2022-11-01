#include "ui_manager.h"
#include "button.h"

namespace smlt {
namespace ui {

Button::Button(UIManager *owner, UIConfig *config, Stage* stage, std::shared_ptr<WidgetStyle> shared_style):
    Widget(owner, config, stage, shared_style) {

    set_resize_mode(config->button_resize_mode_);

    if(!shared_style) {
        set_padding(
            config->button_padding_.left,
            config->button_padding_.right,
            config->button_padding_.bottom,
            config->button_padding_.top
        );

        set_text_colour(config->button_text_colour_);
        set_background_colour(config->button_background_colour_);
        set_foreground_colour(config->button_foreground_colour_);
        set_border_colour(config->button_border_colour_);
        set_border_width(config->button_border_width_);
    }
}



}
}
