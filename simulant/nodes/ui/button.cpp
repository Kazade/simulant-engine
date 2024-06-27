#include "button.h"
#include "simulant/nodes/ui/widget.h"
#include "simulant/utils/construction_args.h"
#include "ui_manager.h"

namespace smlt {
namespace ui {

Button::Button(Scene* owner) :
    Widget(owner, STAGE_NODE_TYPE_WIDGET_BUTTON) {}

bool Button::on_create(const ConstructionArgs& params) {
    if(!Widget::on_create(params)) {
        return false;
    }

    auto sstyle = params.arg<WidgetStyle>("shared_style");
    auto theme = params.arg<UIConfig>("theme").value_or(UIConfig());
    if(!sstyle) {
        set_padding(theme.button_padding_.left, theme.button_padding_.right,
                    theme.button_padding_.bottom, theme.button_padding_.top);

        set_text_color(theme.button_text_color_);
        set_background_color(theme.button_background_color_);
        set_foreground_color(theme.button_foreground_color_);
        set_border_color(theme.button_border_color_);
        set_border_width(theme.button_border_width_);
    }

    auto text = params.arg<unicode>("text").value_or("");
    auto w = params.arg<int>("width").value_or(-1);
    auto h = params.arg<int>("height").value_or(-1);

    set_text(text);
    set_resize_mode(RESIZE_MODE_FIXED_HEIGHT);

    resize(w, h);

    return true;
}
} // namespace ui
} // namespace smlt
