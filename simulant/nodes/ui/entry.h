#pragma once

#include "widget.h"

namespace smlt {
namespace ui {

/** A single-line text entry widget. */
class Entry:
    public Widget {

public:
    Entry(UIManager* owner, UIConfig* config);

};


}
}
