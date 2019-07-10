#pragma once

#include "widget.h"

namespace smlt {
namespace ui {

class Label:
    public Widget {

public:
    using Widget::init; // Pull in init to satisfy Managed<Button>
    using Widget::cleanup;

    Label(WidgetID id, UIManager* owner, UIConfig* config);
};

}
}
