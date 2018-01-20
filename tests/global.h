#ifndef GLOBAL_H
#define GLOBAL_H

#ifdef _arch_dreamcast
#include "simulant/kos_window.h"
extern smlt::KOSWindow::ptr window;
#else
#include "simulant/sdl2_window.h"
extern smlt::SDL2Window::ptr window;
#endif

#include "kaztest/kaztest.h"
#include "simulant/window.h"

class SimulantTestCase : public TestCase {
public:
    void set_up() {
        if(!window) {
#ifdef _arch_dreamcast
            window = smlt::KOSWindow::create(nullptr, 640, 480, 32, false, true);
#else
            window = smlt::SDL2Window::create(nullptr, 640, 480, 0, false, true);
#endif
            window->_init();
            window->set_logging_level(smlt::LOG_LEVEL_NONE);

            auto root = kfs::path::dir_name(kfs::path::dir_name(__FILE__));
            window->resource_locator->add_search_path(
                kfs::path::join(root, "samples/data")
            );

        } else {
            window->reset();
        }
    }
};


#endif // GLOBAL_H
