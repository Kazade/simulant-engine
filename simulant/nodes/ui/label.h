#pragma once

#include "simulant/nodes/stage_node.h"
#include "ui_config.h"
#include "widget.h"

namespace smlt {
namespace ui {

class Label: public Widget, public RefCounted<Label> {

public:
    S_DEFINE_STAGE_NODE_META(STAGE_NODE_TYPE_WIDGET_LABEL);
    S_DEFINE_STAGE_NODE_PARAM(Label, "text", unicode, no_value,
                              "The text to display in the label");
    S_DEFINE_CORE_WIDGET_PROPERTIES(Label);

    Label(Scene* owner);

    bool on_create(Params params) override;
};

} // namespace ui
} // namespace smlt
