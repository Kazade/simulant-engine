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
        set_background_color(config.label_background_color_);
        set_foreground_color(config.label_foreground_color_);
        set_text_color(config.label_text_color_);
        set_border_color(config.label_border_color_);

        set_text(args->text);
        resize(args->width, args->height);
    }

    return true;
}


}
}
