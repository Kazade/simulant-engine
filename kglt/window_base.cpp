#include "utils/glcompat.h"

#include <thread>

#include "utils/gl_error.h"
#include "window_base.h"
#include "ui/interface.h"
#include "input_controller.h"
#include "loaders/texture_loader.h"
#include "loaders/material_script.h"
#include "loaders/q2bsp_loader.h"
#include "loaders/opt_loader.h"
#include "loaders/ogg_loader.h"
#include "loaders/rml_loader.h"
#include "loaders/obj_loader.h"
#include "loaders/tiled_loader.h"
#include "loaders/particle_script.h"

#include "sound.h"
#include "camera.h"
#include "lua/console.h"
#include "watcher.h"
#include "message_bar.h"
#include "render_sequence.h"
#include "stage.h"
#include "ui_stage.h"
#include "virtual_gamepad.h"
#include "physics/physics_engine.h"
#include "screens/loading.h"
#include "utils/gl_thread_check.h"
#include "utils/vao_abstraction.h"

namespace kglt {

WindowBase::WindowBase():
    Source(this),
    BackgroundManager(this),
    StageManager(this),
    UIStageManager(this),
    CameraManager(this),
    ResourceManagerImpl(this),
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
            return loader_type->loader_for(final_file, resource_locator().read_file(final_file));
        }
    }

    throw DoesNotExist<Loader>((_u("Unable to find a loader for: ") + filename).encode());
}

void WindowBase::create_defaults() {
    L_INFO("Initializing the default UI stage");
    create_default_ui_stage();

    //Create a default viewport
    default_viewport_ = new_viewport();
    viewport(default_viewport_)->set_position(0, 0);
    viewport(default_viewport_)->set_size(this->width(), this->height());

    //Host the defaut camera in the default stage
    stage()->host_camera(default_camera_id());

    //Create a default pipeline for the default stage with the default camera
    render_sequence()->new_pipeline(default_stage_id(), default_camera_id());

    default_ui_camera_id_ = new_camera();

    camera(default_ui_camera_id_)->set_orthographic_projection(
        0, this->width(), this->height(), 0, -1, 1
    );

    //Add a pipeline for the default UI stage to render
    //after the main pipeline
    render_sequence()->new_pipeline(
        default_ui_stage_id(), default_ui_camera_id_,
        ViewportID(), TextureID(), 100
    );

    message_bar_ = MessageBar::create(*this);

    loading_ = screens::Loading::create(*this);

    //Weirdly, I had to pass the raw loading pointer here, otherwise some reference was held somewhere even after calling disconnect on the
    //signal and wiping out the connection.
    if(loading_update_connection_.is_connected()) {
        loading_update_connection_.disconnect();
    }

    loading_update_connection_ = signal_step().connect(std::bind(&screens::Loading::update, loading_.get(), std::placeholders::_1));

    //This needs to happen after SDL or whatever is initialized
    input_controller_ = InputController::create(*this);

    console_ = Console::create(*this);
}

bool WindowBase::_init(int width, int height, int bpp, bool fullscreen) {
    GLThreadCheck::init();

    set_width(width);
    set_height(height);

    bool result = create_window(width, height, bpp, fullscreen);

    vao_init(); //Initialize our VAO abstraction layer

    if(result && !initialized_) {        


        //watcher_ = Watcher::create(*this);

        L_INFO("Registering loaders");

        //Register the default resource loaders
        register_loader(std::make_shared<kglt::loaders::TextureLoaderType>());
        register_loader(std::make_shared<kglt::loaders::MaterialScriptLoaderType>());
        register_loader(std::make_shared<kglt::loaders::KGLPLoaderType>());
        register_loader(std::make_shared<kglt::loaders::OPTLoaderType>());
        register_loader(std::make_shared<kglt::loaders::OGGLoaderType>());
        register_loader(std::make_shared<kglt::loaders::RMLLoaderType>());
        register_loader(std::make_shared<kglt::loaders::Q2BSPLoaderType>());
        register_loader(std::make_shared<kglt::loaders::OBJLoaderType>());
        register_loader(std::make_shared<kglt::loaders::TiledLoaderType>());

        L_INFO("Initializing OpenAL");
        Sound::init_openal();

        L_INFO("Initializing the default resources");
        ResourceManagerImpl::init();

        create_defaults();

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

        initialized_ = true;
    }

    return result;
}

void WindowBase::set_logging_level(LoggingLevel level) {
    logging::get_logger("/")->set_level((logging::LOG_LEVEL) level);
}

void WindowBase::update(double dt) {
    if(is_paused()) {
        dt = 0.0; //If the application window is not displayed, don't send a deltatime down
        //it's still accessible through get_deltatime if the user needs it
    }

    BackgroundManager::update(dt);
    StageManager::update(dt);

    if(has_physics_engine()) {
        physics()->step(dt);
    }
}

bool WindowBase::run_frame() {
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

        signal_step_(fixed_step); //Trigger any steps
    }

    fixed_step_interp_ = ktiGetAccumulatorValue();
    signal_post_step_(fixed_step_interp_);

    idle_.execute(); //Execute idle tasks before render

    /* Don't run the render sequence if we don't have a context, and don't update the resource
     * manager either because that probably needs a context too! */
    {
        std::lock_guard<std::mutex> rendering_lock(context_lock_);
        if(has_context()) {
            GLCheck(glViewport, 0, 0, width(), height());
            GLCheck(glClear, GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

            render_sequence()->run();

            signal_pre_swap_();

            swap_buffers();

            //std::this_thread::sleep_for(std::chrono::milliseconds(10));

            ResourceManagerImpl::update();
        }
    }

    signal_frame_finished_();

    if(!is_running_) {
        signal_shutdown_();

        watcher_.reset();

        loading_update_connection_.disconnect();
        loading_update_connection_ = sig::connection();

        loading_.reset();

        //Shutdown the input controller
        input_controller_.reset();
    }

    return is_running_;

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

void WindowBase::enable_physics(std::shared_ptr<PhysicsEngine> engine) {
    physics_engine_ = engine;
}

PhysicsEnginePtr WindowBase::physics() {
    if(!physics_engine_) {
        throw std::logic_error("Tried to access the physics engine when one has not been enabled");
    }
    return physics_engine_;
}

const bool WindowBase::has_physics_engine() const {
    return bool(physics_engine_);
}

double WindowBase::fixed_step_interp() const {
    return fixed_step_interp_;
}

void WindowBase::set_paused(bool value) {
    if(value == is_paused_) return;

    if(value) {
        L_INFO("Pausing application");
    } else {
        L_INFO("Unpausing application");
    }

    is_paused_ = value;
}

void WindowBase::set_has_context(bool value) {
    if(value == has_context_) return;

    has_context_ = value;
}

void WindowBase::enable_virtual_joypad(VirtualDPadDirections directions, int button_count, bool flipped) {
    if(virtual_gamepad_) {
        virtual_gamepad_.reset();
    }

    virtual_gamepad_ = VirtualGamepad::create(*this, directions, button_count);
    if(flipped) {
        virtual_gamepad_->flip();
    }
}

void WindowBase::disable_virtual_joypad() {
    virtual_gamepad_.reset();
}

void WindowBase::handle_mouse_motion(int x, int y, bool pos_normalized) {
    if(pos_normalized) {
        x *= width();
        y *= height();
    }

    UIStageManager::apply_func_to_objects([=](UIStage* object) {
        object->__handle_mouse_move(x, y);
    });
}

void WindowBase::handle_mouse_button_down(int button) {
    UIStageManager::apply_func_to_objects([=](UIStage* object) {
        object->__handle_mouse_down(button);
    });
}

void WindowBase::handle_mouse_button_up(int button) {
    UIStageManager::apply_func_to_objects([=](UIStage* object) {
        object->__handle_mouse_up(button);
    });
}

void WindowBase::handle_touch_down(int finger_id, int x, int y) {
    UIStageManager::apply_func_to_objects([=](UIStage* object) {
        object->__handle_touch_down(finger_id, x, y);
    });
}

void WindowBase::handle_touch_motion(int finger_id, int x, int y) {
    UIStageManager::apply_func_to_objects([=](UIStage* object) {
        object->__handle_touch_motion(finger_id, x, y);
    });
}

void WindowBase::handle_touch_up(int finger_id, int x, int y) {
    UIStageManager::apply_func_to_objects([=](UIStage* object) {
        object->__handle_touch_up(finger_id, x, y);
    });
}

/**
 * @brief WindowBase::reset
 *
 * Destroys all stages, and UI stages, and releases all loadables. Then resets the
 * window to its original state.
 */
void WindowBase::reset() {
    render_sequence()->delete_all_pipelines();

    CameraManager::manager_delete_all();
    UIStageManager::manager_delete_all();
    StageManager::manager_delete_all();
    ViewportManager::manager_delete_all();
    BackgroundManager::manager_delete_all();

    //render_sequence_.reset(new RenderSequence(*this));

    create_default_stage();
    create_default_camera();
    create_defaults();
}

}
