#ifndef GLOBAL_H
#define GLOBAL_H

#include "kglt/window.h"

extern kglt::Window::ptr window;

#include "kaztest/kaztest.h"
#include <kazbase/logging.h>
#include "kglt/window_base.h"

class KGLTTestCase : public TestCase {
public:
    void set_up() {
        logging::get_logger("/")->set_level(logging::LOG_LEVEL_NONE);

        if(!window) {
            window = kglt::Window::create(nullptr);
            window->set_logging_level(kglt::LOG_LEVEL_NONE);
        } else {
            window->reset();
        }
    }
};


#endif // GLOBAL_H
