#pragma once

#include "widget.h"

namespace smlt {
namespace ui {

class Label:
    public Widget {

public:
    Label(UIManager* owner, UIConfig* config, Stage* stage);
};

}
}
