#ifndef GLOBAL_H
#define GLOBAL_H

#include "kglt/sdl2_window.h"

extern kglt::SDL2Window::ptr window;

#include "kaztest/kaztest.h"
#include "kglt/window_base.h"

class KGLTTestCase : public TestCase {
public:
    void set_up() {
        if(!window) {
            window = kglt::SDL2Window::create(nullptr);
            window->set_logging_level(kglt::LOG_LEVEL_NONE);
        } else {
            window->reset();
        }
    }
};


#endif // GLOBAL_H
