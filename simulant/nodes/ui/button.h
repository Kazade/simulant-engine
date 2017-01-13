#pragma once

#include "../../generic/managed.h"
#include "widget.h"

namespace smlt {
namespace ui {

class Button:
    public Widget,
    public Managed<Button> {

public:
    using Widget::init; // Pull in init to satisfy Managed<Button>
    using StageNode::cleanup;

    Button(WidgetID id, UIManager* owner, UIConfig* config):
        Widget(id, owner, config) {}
};

}
}
