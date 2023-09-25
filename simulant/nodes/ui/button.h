#pragma once

#include "../../generic/managed.h"
#include "widget.h"

namespace smlt {
namespace ui {

struct ButtonParams : public WidgetParams {
    unicode text;
    Px width = Px(-1);
    Px height = Rem(1.5f);

    ButtonParams(
        const unicode& text,
        const Px& width=-1,
        const Px& height=-1,
        const UIConfig& theme=UIConfig(),
        WidgetStylePtr shared_style=WidgetStylePtr()
    ):
        WidgetParams(theme, shared_style),
        text(text),
        width(width),
        height(height) {}
};

class Button:
    public Widget {

public:
    struct Meta {
        typedef ui::ButtonParams params_type;
        const static StageNodeType node_type = STAGE_NODE_TYPE_WIDGET_BUTTON;
    };

    Button(Scene *owner);
    bool on_create(void* params) override;
};

}
}
