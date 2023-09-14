#pragma once

#include "widget.h"

namespace smlt {
namespace ui {

struct LabelParams {
    UIConfig config;
    unicode text;
    Px width;
    Px height;

    LabelParams(const unicode& text, const Px& width=-1, const Px& height=-1):
        text(text),
        width(width),
        height(height) {}

    LabelParams(const UIConfig& config):
        config(config) {}
};

class Label:
    public Widget,
    public RefCounted<Label> {

public:
    Label(Scene* owner, const UIConfig& config);
};

}

template<>
struct stage_node_traits<ui::Label> {
    typedef ui::LabelParams params_type;
    const static StageNodeType node_type = STAGE_NODE_TYPE_WIDGET_LABEL;
};

}
