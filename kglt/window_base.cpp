#include <GLee.h>
#include <thread>

#include "window_base.h"
#include "scene.h"
#include "input_controller.h"
#include "loaders/texture_loader.h"
#include "loaders/material_script.h"
#include "loaders/q2bsp_loader.h"
#include "loaders/opt_loader.h"
#include "loaders/ogg_loader.h"
#include "loaders/rml_loader.h"
#include "utils/debug_bar.h"
#include "sound.h"

namespace kglt {

WindowBase::WindowBase():
    initialized_(false),
    width_(-1),
    height_(-1),
    is_running_(true),
    default_viewport_(0),
    resource_locator_(ResourceLocator::create()),
    frame_counter_time_(0),
    frame_counter_frames_(0),
    frame_time_in_milliseconds_(0),
    total_time_(0) {

    //Register the default resource loaders
    register_loader(LoaderType::ptr(new kglt::loaders::TextureLoaderType));
    register_loader(LoaderType::ptr(new kglt::loaders::MaterialScriptLoaderType));
    register_loader(LoaderType::ptr(new kglt::loaders::Q2BSPLoaderType));
    register_loader(LoaderType::ptr(new kglt::loaders::OPTLoaderType));
    register_loader(LoaderType::ptr(new kglt::loaders::OGGLoaderType));
    register_loader(LoaderType::ptr(new kglt::loaders::RMLLoaderType));

    ktiGenTimers(1, &timer_);
    ktiBindTimer(timer_);
    ktiStartFixedStepTimer(30);

    ktiGenTimers(1, &frame_timer_);
    ktiBindTimer(frame_timer_);
    ktiStartGameTimer();

    set_logging_level(LOG_LEVEL_NONE);

    logging::get_logger("/")->add_handler(logging::Handler::ptr(new logging::StdIOHandler));   
}

WindowBase::~WindowBase() {
    Sound::shutdown_openal();
}

bool WindowBase::init(int width, int height, int bpp, bool fullscreen) {
    set_width(width);
    set_height(height);

    bool result = create_window(width, height, bpp, fullscreen);        

    Sound::init_openal();

    if(result && !initialized_) {
        debug_bar_.reset(new DebugBar(width_, height_));
        debug_bar().add_read_only_variable("Frame Time (ms)", &frame_time_in_milliseconds_);

        //Create a default viewport
        default_viewport_ = new_viewport();
        viewport(default_viewport_).set_position(0, 0);
        viewport(default_viewport_).set_size(this->width(), this->height());

        scene_ = Scene::create(this);

        //This needs to happen after SDL or whatever is initialized
        input_controller_ = InputController::create();

        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LEQUAL);
        glEnable(GL_MULTISAMPLE);

        glHint(GL_LINE_SMOOTH_HINT, GL_NICEST );
        glHint(GL_POLYGON_SMOOTH_HINT, GL_NICEST );

        glEnable(GL_POLYGON_SMOOTH);
        glEnable(GL_LINE_SMOOTH);
        glEnable(GL_POINT_SMOOTH);

        glEnable(GL_CULL_FACE);

        using std::bind;

        //C++11 lambda awesomeness! input_controller isn't initialized yet
        //so we connect ESCAPE in an idle task
        idle().add_once([=]() {
            //Bind the stop_running method to the ESCAPE key
            input_controller().keyboard().key_pressed_connect(
                KEY_CODE_ESCAPE, bind(&WindowBase::stop_running, this)
            );
        });

        idle().add_once([=]() {
            DebugBar* bar = &debug_bar();
            input_controller().keyboard().key_pressed_connect(
                KEY_CODE_BACKQUOTE, bind(&DebugBar::toggle, bar)
            );
        });

        initialized_ = true;
    }
    return result;
}

void WindowBase::set_logging_level(LoggingLevel level) {
    logging::get_logger("/")->set_level((logging::LOG_LEVEL) level);
}

bool WindowBase::update(WindowUpdateCallback step) {
    signal_frame_started_();

    ktiBindTimer(frame_timer_);
    ktiUpdateFrameTime();

    frame_counter_time_ += ktiGetDeltaTime();
    frame_counter_frames_++;

    if(frame_counter_time_ >= 1.0) {
        frame_time_in_milliseconds_ = 1000.0 / double(frame_counter_frames_);
        frame_counter_frames_ = 0;
        frame_counter_time_ = 0.0;
    }

    ktiBindTimer(timer_);
    ktiUpdateFrameTime();

    total_time_ += delta_time();

    idle_.execute(); //Execute idle tasks first
    check_events();

    while(ktiTimerCanUpdate()) {
        input_controller().update(delta_time());

        if(step) {
            step(delta_time());
        }

        scene().update(delta_time());
    }

    glViewport(0, 0, width(), height());
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    scene().render();

    signal_pre_swap_();

    if(debug_bar_) {
        debug_bar_->render();
    }

    swap_buffers();

    //std::this_thread::sleep_for(std::chrono::milliseconds(10));

    if(!is_running_) {
        //Shutdown the input controller
        input_controller_.reset();
        //Destroy the scene
        scene_.reset();
        debug_bar_.reset();
    }

    signal_frame_finished_();

    return is_running_;

}

Scene& WindowBase::scene() {
    if(!scene_) {
        throw DoesNotExist<Scene>();
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
