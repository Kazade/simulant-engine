#include "ui_manager.h"
#include "label.h"

namespace smlt {
namespace ui {

Label::Label(Scene *owner):
    Widget(owner, STAGE_NODE_TYPE_WIDGET_LABEL) {
}

bool Label::on_create(void* params) {
    if(!Widget::on_create(params)) {
        return false;
    }

    LabelParams* args = (LabelParams*) params;
    if(!args->shared_style) {
        auto& config = args->theme;
        set_resize_mode(config.label_resize_mode_);
        set_background_colour(config.label_background_colour_);
        set_foreground_colour(config.label_foreground_colour_);
        set_text_colour(config.label_text_colour_);
        set_border_colour(config.label_border_colour_);

        set_text(args->text);
        resize(args->width, args->height);
    }

    return true;
}


}
}
