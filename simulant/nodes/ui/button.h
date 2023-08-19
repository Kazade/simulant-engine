#pragma once

#include "../../generic/managed.h"
#include "widget.h"

namespace smlt {
namespace ui {

class Button:
    public Widget {

public:
    Button(Scene *owner,
        UIConfig config,
        std::shared_ptr<WidgetStyle> shared_style=std::shared_ptr<WidgetStyle>()
    );
};

}
}
