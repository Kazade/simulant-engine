
#include "button.h"

namespace smlt {
namespace ui {

Button::Button(WidgetID id, UIManager *owner, UIConfig *config):
    Widget(id, owner, config) {

    set_resize_mode(config->button_resize_mode_);
    set_padding(
        config->button_padding_.left,
        config->button_padding_.right,
        config->button_padding_.bottom,
        config->button_padding_.top
    );

    set_background_colour(config->button_background_color_);
    set_foreground_colour(config->button_foreground_color_);
    set_border_colour(config->button_border_colour_);
    set_border_width(config->button_border_width_);
}



}
}
