#pragma once

#include "../../generic/managed.h"
#include "widget.h"

namespace smlt {
namespace ui {

struct ButtonParams {
    UIConfig config;
    unicode text;
    Px width;
    Px height;

    ButtonParams(const unicode& text, const Px& width=-1, const Px& height=-1):
        text(text),
        width(width),
        height(height) {}
};

class Button:
    public Widget {

public:
    Button(Scene *owner,
        UIConfig config,
        std::shared_ptr<WidgetStyle> shared_style=std::shared_ptr<WidgetStyle>()
    );
};

}

template<>
struct stage_node_traits<ui::Button> {
    typedef ui::ButtonParams params_type;
    const static StageNodeType node_type = STAGE_NODE_TYPE_WIDGET_BUTTON;
};

}
