#include "label.h"
#include "ui_manager.h"

namespace smlt {
namespace ui {

Label::Label(Scene* owner) :
    Widget(owner, STAGE_NODE_TYPE_WIDGET_LABEL) {}

bool Label::on_create(Params params) {
    if(!clean_params<Label>(params)) {
        return false;
    }

    if(!Widget::on_create(params)) {
        return false;
    }

    auto shared_style =
        params.get<WidgetStylePtr>("shared_style").value_or(WidgetStylePtr());

    if(!shared_style) {
        auto config = params.get<UIConfig>("theme").value_or(UIConfig());
        set_resize_mode(config.label_resize_mode_);
        set_background_color(config.label_background_color_);
        set_foreground_color(config.label_foreground_color_);
        set_text_color(config.label_text_color_);
        set_border_color(config.label_border_color_);

        unicode txt = params.get<std::string>("text").value_or("");
        set_text(txt);

        auto width = params.get<int>("width").value_or(-1);
        auto height = params.get<int>("height").value_or(-1);
        resize(width, height);
    }

    return true;
}
} // namespace ui
} // namespace smlt
