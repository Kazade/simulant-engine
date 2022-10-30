#pragma once

#include "widget.h"

namespace smlt {
namespace ui {

class Label:
    public Widget,
    public RefCounted<Label> {

public:
    Label(UIManager* owner, UIConfig* config, Stage* stage);
};

}
}
