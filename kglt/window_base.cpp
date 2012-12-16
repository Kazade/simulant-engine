#include <boost/thread/thread.hpp>

#include "glee/GLee.h"
#include "window_base.h"
#include "scene.h"

namespace kglt {

WindowBase::WindowBase():
    initialized_(false),
    width_(0),
    height_(0),
    is_running_(true),
    default_viewport_(0),
    resource_locator_(ResourceLocator::create()) {

    //Register the default resource loaders
    register_loader(LoaderType::ptr(new kglt::loaders::TextureLoaderType));
    register_loader(LoaderType::ptr(new kglt::loaders::MaterialScriptLoaderType));
    register_loader(LoaderType::ptr(new kglt::loaders::Q2BSPLoaderType));

    ktiGenTimers(1, &timer_);
    ktiBindTimer(timer_);
    ktiStartGameTimer();
}

void WindowBase::init() {    
    if(!initialized_) {
        assert(width_ && "Subclass should've set the window width by now");
        assert(height_ && "Subclass should've set the window height by now");

        //Create a default viewport
        default_viewport_ = new_viewport();
        viewport(default_viewport_).set_position(0, 0);
        viewport(default_viewport_).set_size(width(), height());

        //Initialize the scene
        scene().init();
        initialized_ = true;
    }
}

bool WindowBase::update() {    
    init(); //Make sure we were initialized

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

ViewportID WindowBase::new_viewport() {
    return TemplatedManager<WindowBase, Viewport, ViewportID>::manager_new();
}

Viewport& WindowBase::viewport(ViewportID viewport) {
    if(viewport == 0) {
        return TemplatedManager<WindowBase, Viewport, ViewportID>::manager_get(default_viewport_);
    }
    return TemplatedManager<WindowBase, Viewport, ViewportID>::manager_get(viewport);
}

void WindowBase::delete_viewport(ViewportID viewport) {
    TemplatedManager<WindowBase, Viewport, ViewportID>::manager_delete(viewport);
}


}
