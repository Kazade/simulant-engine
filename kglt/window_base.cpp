#include "window_base.h"

namespace kglt {
    
bool WindowBase::update() {
    static bool initialized = false;
    if(!initialized) {
        assert(width_ && "Subclass should've set the window width by now");
        assert(height_ && "Subclass should've set the window height by now");
    
        scene().init();
        initialized = true;
    }
    
    check_events();

    scene().viewport().update_opengl();
    
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	scene().update(0.01);

    scene().render();

    swap_buffers();

    return is_running_;
}

void WindowBase::register_loader(LoaderType::ptr loader) {
    //FIXME: assert doesn't exist already
    loaders_.push_back(loader);
}


}
