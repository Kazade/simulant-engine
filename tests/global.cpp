#include "global.h"

#ifdef _arch_dreamcast
smlt::KOSWindow::ptr window;
#else
smlt::SDL2Window::ptr window;
#endif

void SimulantTestCase::set_up() {
    if(!window) {
        kazlog::get_logger("/")->add_handler(kazlog::Handler::ptr(new kazlog::StdIOHandler));

#ifdef _arch_dreamcast
        window = smlt::KOSWindow::create(nullptr, 640, 480, 32, false, true);
#else
        window = smlt::SDL2Window::create(nullptr, 640, 480, 0, false, true);
#endif
        window->_init();

        if(std::getenv("SIMULANT_DEBUG")) {
            window->set_logging_level(smlt::LOG_LEVEL_DEBUG);
        } else {
            window->set_logging_level(smlt::LOG_LEVEL_NONE);
        }

        window->resource_locator->add_search_path("sample_data");
    } else {
        window->reset();
    }
}
