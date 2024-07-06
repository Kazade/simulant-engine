#pragma once

#include "../../generic/managed.h"
#include "simulant/nodes/stage_node.h"
#include "simulant/utils/params.h"
#include "widget.h"

namespace smlt {
namespace ui {

class Button: public Widget {

public:
    S_DEFINE_STAGE_NODE_META(STAGE_NODE_TYPE_WIDGET_BUTTON);
    S_DEFINE_STAGE_NODE_PARAM(Button, "text", std::string, no_value,
                              "The text to display in the button");
    S_DEFINE_CORE_WIDGET_PROPERTIES(Button);

    Button(Scene* owner);
    bool on_create(Params params) override;
};

} // namespace ui
} // namespace smlt
