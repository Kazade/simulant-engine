#pragma once

#include "../../generic/managed.h"
#include "widget.h"

namespace smlt {
namespace ui {

class Button:
    public Widget {

public:
    using Widget::init; // Pull in init to satisfy Managed<Button>
    using Widget::clean_up;

    Button(WidgetID id, UIManager* owner, UIConfig* config);
};

}
}
