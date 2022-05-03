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

    Button(UIManager* owner, UIConfig* config, std::shared_ptr<WidgetStyle> shared_style=std::shared_ptr<WidgetStyle>());
};

}
}
