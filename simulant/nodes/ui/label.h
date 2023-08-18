#pragma once

#include "widget.h"

namespace smlt {
namespace ui {

struct LabelParams {
    UIConfig config;
    unicode text;
};

class Label:
    public Widget,
    public RefCounted<Label> {

public:
    Label(UIManager* owner, UIConfig* config, Stage* stage);
};

}

template<>
struct stage_node_traits<ui::Label> {
    typedef ui::LabelParams params_type;
    const static StageNodeType node_type = STAGE_NODE_TYPE_WIDGET_LABEL;
};

}
