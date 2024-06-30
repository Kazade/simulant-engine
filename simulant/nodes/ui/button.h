#pragma once

#include "../../generic/managed.h"
#include "simulant/nodes/stage_node.h"
#include "simulant/utils/construction_args.h"
#include "widget.h"

namespace smlt {
namespace ui {

class Button: public Widget {

public:
    S_DEFINE_STAGE_NODE_META(STAGE_NODE_TYPE_WIDGET_BUTTON);

    Button(Scene* owner);
    bool on_create(const Params& params) override;
};

} // namespace ui
} // namespace smlt
