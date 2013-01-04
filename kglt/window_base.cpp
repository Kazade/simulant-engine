#include <thread>

#include "glee/GLee.h"
#include "window_base.h"
#include "scene.h"
#include "input_controller.h"

namespace kglt {

WindowBase::WindowBase():
    initialized_(false),
    width_(-1),
    height_(-1),
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

void WindowBase::init_window() {
    if(!initialized_) {
        assert(width_ > -1 && "Subclass should've set the window width by now");
        assert(height_ > -1 && "Subclass should've set the window height by now");

        //Create a default viewport
        default_viewport_ = new_viewport();
        viewport(default_viewport_).set_position(0, 0);
        viewport(default_viewport_).set_size(width(), height());

        //Initialize the scene
        scene().init();

        //This needs to happen after SDL or whatever is initialized
        input_controller_ = InputController::create();

        initialized_ = true;
    }
}

bool WindowBase::update() {    
    init_window(); //Make sure we were initialized

    idle_.execute(); //Execute idle tasks first   
    check_events();

    ktiBindTimer(timer_);
    ktiUpdateFrameTime();

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);

    glViewport(0, 0, width(), height());
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    scene().update(delta_time());

    scene().render();

    swap_buffers();

    //std::this_thread::sleep_for(std::chrono::milliseconds(10));
    
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

Keyboard& WindowBase::keyboard() {
    return input_controller_->keyboard();
}

Mouse& WindowBase::mouse() {
    return input_controller_->mouse();
}

Joypad& WindowBase::joypad(uint8_t idx) {
    return input_controller_->joypad(idx);
}

uint8_t WindowBase::joypad_count() const {
    return input_controller_->joypad_count();
}

}
