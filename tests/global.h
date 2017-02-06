#ifndef GLOBAL_H
#define GLOBAL_H

#include "simulant/sdl2_window.h"

extern smlt::SDL2Window::ptr window;

#include "kaztest/kaztest.h"
#include "simulant/window_base.h"

class SimulantTestCase : public TestCase {
public:
    void set_up() {
        if(!window) {
            window = smlt::SDL2Window::create(nullptr);
            window->_init();
            window->set_logging_level(smlt::LOG_LEVEL_NONE);
        } else {
            window->reset();
        }
    }
};


#endif // GLOBAL_H
