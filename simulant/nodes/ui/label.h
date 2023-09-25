#pragma once

#include "widget.h"

namespace smlt {
namespace ui {

struct LabelParams : public WidgetParams {
    unicode text;
    Px width;
    Px height;

    LabelParams(
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

class Label:
    public Widget,
    public RefCounted<Label> {

public:
    struct Meta {
        typedef ui::LabelParams params_type;
        const static StageNodeType node_type = STAGE_NODE_TYPE_WIDGET_LABEL;
    };

    Label(Scene* owner);

    bool on_create(void *params) override;
};

}
}
