#pragma once

#include "widget.h"

namespace smlt {
namespace ui {

class ProgressBar:
    public Widget,
    public Managed<ProgressBar> {

public:
    using Widget::init; // Pull in init to satisfy Managed<Button>
    using StageNode::cleanup;

    ProgressBar(WidgetID id, UIManager* owner, UIConfig* config):
        Widget(id, owner, config) {}
};

}
}
