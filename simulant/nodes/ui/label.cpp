#include "label.h"

namespace smlt {
namespace ui {

Label::Label(WidgetID id, UIManager *owner, UIConfig *config):
    Widget(id, owner, config) {

    set_resize_mode(config->label_resize_mode_);
    set_background_colour(config->label_background_colour_);
    set_foreground_colour(config->label_foreground_colour_);
    set_text_colour(config->label_text_colour_);
    set_border_colour(config->label_border_colour_);
}


}
}
