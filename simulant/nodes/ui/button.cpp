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

        set_text_colour(args->theme.button_text_colour_);
        set_background_colour(args->theme.button_background_colour_);
        set_foreground_colour(args->theme.button_foreground_colour_);
        set_border_colour(args->theme.button_border_colour_);
        set_border_width(args->theme.button_border_width_);
    }

    set_resize_mode(RESIZE_MODE_FIXED_HEIGHT);
    resize(args->width, args->height);

    return true;
}



}
}
