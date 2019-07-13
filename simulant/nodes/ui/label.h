#pragma once

#include "widget.h"

namespace smlt {
namespace ui {

class Label:
    public Widget {

public:
    Label(WidgetID id, UIManager* owner, UIConfig* config);
};

}
}
