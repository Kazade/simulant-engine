#include "utils/glcompat.h"

#include <thread>

#include "utils/gl_error.h"
#include "window_base.h"
#include "scene.h"
#include "ui/interface.h"
#include "input_controller.h"
#include "loaders/texture_loader.h"
#include "loaders/material_script.h"
#include "loaders/q2bsp_loader.h"
#include "loaders/opt_loader.h"
#include "loaders/ogg_loader.h"
#include "loaders/rml_loader.h"
#include "loaders/obj_loader.h"
#include "sound.h"
#include "lua/console.h"
#include "watcher.h"
#include "message_bar.h"
#include "render_sequence.h"

#include "screens/loading.h"
#include "utils/gl_thread_check.h"

namespace kglt {

WindowBase::WindowBase():
    Source(this),
    BackgroundManager(this),
    initialized_(false),
    width_(-1),
    height_(-1),
    is_running_(true),
    idle_(*this),
    default_viewport_(0),
    resource_locator_(ResourceLocator::create()),
    frame_counter_time_(0),
    frame_counter_frames_(0),
    frame_time_in_milliseconds_(0),
    total_time_(0),
    render_sequence_(new RenderSequence(*this)) {

    ktiGenTimers(1, &fixed_timer_);
    ktiBindTimer(fixed_timer_);
    ktiStartFixedStepTimer(WindowBase::STEPS_PER_SECOND);

    ktiGenTimers(1, &variable_timer_);
    ktiBindTimer(variable_timer_);
    ktiStartGameTimer();

    set_logging_level(LOG_LEVEL_NONE);

    logging::get_logger("/")->add_handler(logging::Handler::ptr(new logging::StdIOHandler));       
}

WindowBase::~WindowBase() {
    //FIXME: Make WindowBase Managed<> and put this in cleanup()
    loading_.reset();
    message_bar_.reset();
    console_.reset();
    watcher_.reset();

    Sound::shutdown_openal();
}

RenderSequencePtr WindowBase::render_sequence() {
    return render_sequence_;
}

LoaderPtr WindowBase::loader_for(const unicode &filename) {
    unicode final_file = resource_locator().locate_file(filename);

    for(LoaderTypePtr loader_type: loaders_) {
        if(loader_type->supports(final_file)) {
            return loader_type->loader_for(final_file.encode());
        }
    }

    throw DoesNotExist<Loader>((_u("Unable to find a loader for: ") + filename).encode());
}

bool WindowBase::init(int width, int height, int bpp, bool fullscreen) {
    GLThreadCheck::init();
    check_and_log_error(__FILE__, __LINE__);

    set_width(width);
    set_height(height);

    bool result = create_window(width, height, bpp, fullscreen);        

    Sound::init_openal();

    if(result && !initialized_) {
        watcher_ = Watcher::create(*this);

        //Register the default resource loaders
        register_loader(std::make_shared<kglt::loaders::TextureLoaderType>());
        register_loader(std::make_shared<kglt::loaders::MaterialScriptLoaderType>());
        register_loader(std::make_shared<kglt::loaders::OPTLoaderType>());
        register_loader(std::make_shared<kglt::loaders::OGGLoaderType>());
        register_loader(std::make_shared<kglt::loaders::RMLLoaderType>());
        register_loader(std::make_shared<kglt::loaders::Q2BSPLoaderType>());
        register_loader(std::make_shared<kglt::loaders::OBJLoaderType>());

        //Create a default viewport
        default_viewport_ = new_viewport();
        viewport(default_viewport_)->set_position(0, 0);
        viewport(default_viewport_)->set_size(this->width(), this->height());

        scene_ = SceneImpl::create(this);

        /*FIXME: This should be called in Scene::init, but because Scene subclasses ResourceManagerImpl,
         * which takes a WindowBase, rather than a Scene, we try to do window().scene() which crashes because
         * that isn't set until create returns (above). ResourceManagerImpl should probably take a Scene*
         * and do window lookups with scene()->window() instead of the reverse. */
        scene_->initialize_defaults();

        message_bar_ = MessageBar::create(*this);

        loading_ = screens::Loading::create(this->scene());

        //Weirdly, I had to pass the raw loading pointer here, otherwise some reference was held somewhere even after calling disconnect on the
        //signal and wiping out the connection.
        loading_update_connection_ = signal_step().connect(std::bind(&screens::Loading::update, loading_.get(), std::placeholders::_1));

        //This needs to happen after SDL or whatever is initialized
        input_controller_ = InputController::create();

        GLCheck(glEnable, GL_DEPTH_TEST);
        GLCheck(glDepthFunc, GL_LEQUAL);
        GLCheck(glEnable, GL_CULL_FACE);

        using std::bind;

        //C++11 lambda awesomeness! input_controller isn't initialized yet
        //so we connect ESCAPE in an idle task
        idle().add_once([=]() {
            //Bind the stop_running method to the ESCAPE key
            input_controller().keyboard().key_pressed_connect(
                SDL_SCANCODE_ESCAPE, bind(&WindowBase::stop_running, this)
            );
        });

        console_ = Console::create(*this);

        initialized_ = true;
    }
    check_and_log_error(__FILE__, __LINE__);
    return result;
}

void WindowBase::set_logging_level(LoggingLevel level) {
    logging::get_logger("/")->set_level((logging::LOG_LEVEL) level);
}

bool WindowBase::run_frame() {
    check_and_log_error(__FILE__, __LINE__);

    signal_frame_started_();

    ktiBindTimer(variable_timer_);
    ktiUpdateFrameTime();

    delta_time_ = ktiGetDeltaTime();
    total_time_ += delta_time_;

    frame_counter_time_ += delta_time_;
    frame_counter_frames_++;

    if(frame_counter_time_ >= 1.0) {
        frame_time_in_milliseconds_ = 1000.0 / double(frame_counter_frames_);
        frame_counter_frames_ = 0;
        frame_counter_time_ = 0.0;
    }

    //Update any playing sounds
    update_source(delta_time_);

    ktiBindTimer(fixed_timer_);
    ktiUpdateFrameTime();
    double fixed_step = ktiGetDeltaTime();

    check_events();

    while(ktiTimerCanUpdate()) {
        idle_.execute(); //Execute idle tasks before render

        input_controller().update(fixed_step);

        update(fixed_step); //Update this

        scene().update(fixed_step);

        signal_step_(fixed_step); //Trigger any steps        
    }

    idle_.execute(); //Execute idle tasks before render

    GLCheck(glViewport, 0, 0, width(), height());
    GLCheck(glClear, GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
    check_and_log_error(__FILE__, __LINE__);

    render_sequence()->run();

    signal_pre_swap_();

    swap_buffers();

    //std::this_thread::sleep_for(std::chrono::milliseconds(10));

    signal_frame_finished_();

    if(!is_running_) {
        signal_shutdown_();

        watcher_.reset();

        loading_update_connection_.disconnect();
        loading_update_connection_ = sig::connection();

        loading_.reset();

        //Shutdown the input controller
        input_controller_.reset();        
        //Destroy the scene
        scene_.reset();
    }

    return is_running_;

}

const Scene& WindowBase::scene() const {
    if(!scene_) {
        throw DoesNotExist<Scene>();
    }
    return *scene_;
}

Scene& WindowBase::scene() {
    if(!scene_) {
        throw DoesNotExist<Scene>();
    }
    return *scene_;
}

void WindowBase::register_loader(LoaderTypePtr loader) {
    if(container::contains(loaders_, loader)) {
        throw LogicError("Tried to add the same loader twice");
    }

    loaders_.push_back(loader);
}

ViewportID WindowBase::new_viewport() {
    return ViewportManager::manager_new();
}

ViewportPtr WindowBase::viewport(ViewportID viewport) {
    if(!viewport) {
        return ViewportManager::manager_get(default_viewport_);
    }
    return ViewportManager::manager_get(viewport);
}

void WindowBase::delete_viewport(ViewportID viewport) {
    ViewportManager::manager_delete(viewport);
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
