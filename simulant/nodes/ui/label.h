#pragma once

#include "widget.h"

namespace smlt {
namespace ui {

struct LabelParams {
    UIConfig config;
    unicode text;

    LabelParams(const unicode& text):
        text(text) {}

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
