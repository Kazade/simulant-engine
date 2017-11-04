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
    using Widget::cleanup;

    Button(WidgetID id, UIManager* owner, UIConfig* config);
};

}
}
