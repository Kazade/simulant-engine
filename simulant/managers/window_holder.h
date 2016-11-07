#pragma once

#include "../generic/property.h"

namespace smlt {

class WindowBase;

class WindowHolder {
public:
    WindowHolder(WindowBase* window):
        window_(window) {}

    Property<WindowHolder, WindowBase> window = { this, &WindowHolder::window_ };

private:
    WindowBase* window_;
};

}
