#include "ui_manager.h"
#include "button.h"

namespace smlt {
namespace ui {

Button::Button(Scene *owner):
    Widget(owner, STAGE_NODE_TYPE_WIDGET_BUTTON) {


}

bool Button::on_create(void* params) {
    if(!Widget::on_create(params)) {
        return false;
    }

    ButtonParams* args = (ButtonParams*) params;

    if(!args->shared_style) {
        set_padding(
            args->theme.button_padding_.left,
            args->theme.button_padding_.right,
            args->theme.button_padding_.bottom,
            args->theme.button_padding_.top
        );

        set_text_color(args->theme.button_text_color_);
        set_background_color(args->theme.button_background_color_);
        set_foreground_color(args->theme.button_foreground_color_);
        set_border_color(args->theme.button_border_color_);
        set_border_width(args->theme.button_border_width_);
    }

    set_text(args->text);
    set_resize_mode(RESIZE_MODE_FIXED_HEIGHT);

    resize(args->width, args->height);

    return true;
}



}
}
