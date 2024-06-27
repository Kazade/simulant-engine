#pragma once

#include "simulant/nodes/stage_node.h"
#include "widget.h"

namespace smlt {
namespace ui {

class Label:
    public Widget,
    public RefCounted<Label> {

public:
    S_DEFINE_STAGE_NODE_META(STAGE_NODE_TYPE_WIDGET_LABEL);

    Label(Scene* owner);

    bool on_create(ConstructionArgs* params) override;
};

}
}
