#include <boost/thread/thread.hpp>

#include "glee/GLee.h"
#include "window_base.h"
#include "scene.h"

namespace kglt {
    
bool WindowBase::update() {
    static bool initialized = false;
    if(!initialized) {
        assert(width_ && "Subclass should've set the window width by now");
        assert(height_ && "Subclass should've set the window height by now");
    
        scene().init();
        initialized = true;
    }
    
    idle_.execute(); //Execute idle tasks first   
    check_events();

    ktiUpdateFrameTime();

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    scene().update(delta_time());

    scene().render();

    swap_buffers();

    boost::this_thread::sleep(boost::posix_time::milliseconds(10));
    
    return is_running_;
}

Scene& WindowBase::scene() {
    if(!scene_) {
        scene_.reset(new Scene(this));
    }
    return *scene_;
}

void WindowBase::register_loader(LoaderType::ptr loader) {
    //FIXME: assert doesn't exist already
    loaders_.push_back(loader);
}


}
