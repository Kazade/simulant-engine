//
//   Copyright (c) 2011-2017 Luke Benstead https://simulant-engine.appspot.com
//
//     This file is part of Simulant.
//
//     Simulant is free software: you can redistribute it and/or modify
//     it under the terms of the GNU General Public License as published by
//     the Free Software Foundation, either version 3 of the License, or
//     (at your option) any later version.
//
//     Simulant is distributed in the hope that it will be useful,
//     but WITHOUT ANY WARRANTY; without even the implied warranty of
//     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//     GNU General Public License for more details.
//
//     You should have received a copy of the GNU General Public License
//     along with Simulant.  If not, see <http://www.gnu.org/licenses/>.
//

#include <thread>

#ifdef _arch_dreamcast
    #include <kos.h>
#endif

#include "utils/gl_error.h"
#include "window_base.h"
#include "input_controller.h"
#include "loaders/texture_loader.h"
#include "loaders/material_script.h"
#include "loaders/opt_loader.h"
#include "loaders/ogg_loader.h"
#include "loaders/obj_loader.h"
#include "loaders/tiled_loader.h"
#include "loaders/particle_script.h"
#include "loaders/heightmap_loader.h"
#include "loaders/q2bsp_loader.h"
#include "loaders/wal_loader.h"
#include "loaders/md2_loader.h"
#include "loaders/pcx_loader.h"
#include "loaders/ttf_loader.h"

#include "sound.h"
#include "camera.h"
#include "render_sequence.h"
#include "stage.h"
#include "virtual_gamepad.h"
#include "scenes/loading.h"
#include "utils/gl_thread_check.h"
#include "utils/gl_error.h"

#include "panels/stats_panel.h"
#include "panels/partitioner_panel.h"

namespace smlt {

WindowBase::WindowBase():
    Source(this),
    StageManager(this),    
    CameraManager(this),
    resource_manager_(new ResourceManager(this)),
    initialized_(false),
    width_(-1),
    height_(-1),
    is_running_(true),
    idle_(*this),
    resource_locator_(ResourceLocator::create()),
    frame_counter_time_(0),
    frame_counter_frames_(0),
    frame_time_in_milliseconds_(0),
    background_manager_(new BackgroundManager(this)),
    time_keeper_(TimeKeeper::create(1.0 / WindowBase::STEPS_PER_SECOND)) {

}

WindowBase::~WindowBase() {

}

RenderSequencePtr WindowBase::render_sequence() {
    return render_sequence_;
}

LoaderPtr WindowBase::loader_for(const unicode &filename, LoaderHint hint) {
    unicode final_file = resource_locator->locate_file(filename);

    std::vector<std::pair<LoaderTypePtr, LoaderPtr>> possible_loaders;

    for(LoaderTypePtr loader_type: loaders_) {
        if(loader_type->supports(final_file)) {
            auto new_loader = loader_type->loader_for(final_file, resource_locator->read_file(final_file));
            new_loader->set_resource_locator(this->resource_locator_.get());

            possible_loaders.push_back(
                std::make_pair(loader_type, new_loader)
            );
        }
    }

    if(possible_loaders.size() == 1) {
        return possible_loaders.front().second;
    } else if(possible_loaders.size() > 1) {
        if(hint != LOADER_HINT_NONE) {
            for(auto& p: possible_loaders) {
                if(p.first->has_hint(hint)) {
                    return p.second;
                }
            }
        }

        throw std::logic_error(_u("More than one possible loader was found for '{0}'. Please specify a hint.").format(filename).encode());
    }

    return LoaderPtr();
}


LoaderPtr WindowBase::loader_for(const unicode& loader_name, const unicode &filename) {
    unicode final_file = resource_locator->locate_file(filename);

    for(LoaderTypePtr loader_type: loaders_) {
        if(loader_type->name() == loader_name) {
            if(loader_type->supports(final_file)) {
                return loader_type->loader_for(final_file, resource_locator->read_file(final_file));
            } else {
                throw std::logic_error(_u("Loader '{0}' does not support file '{1}'").format(loader_name, filename).encode());
            }
        }
    }

    return LoaderPtr();
}

LoaderTypePtr WindowBase::loader_type(const unicode& loader_name) const {
    for(LoaderTypePtr loader_type: loaders_) {
        if(loader_type->name() == loader_name) {
            return loader_type;
        }
    }
    return LoaderTypePtr();
}

void WindowBase::create_defaults() {
    loading_ = scenes::Loading::create(*this);

    //This needs to happen after SDL or whatever is initialized
    input_controller_ = InputController::create(*this);

    // Tell subclasses to initialize input devices
    initialize_input_controller(*input_controller_);
}

void WindowBase::_cleanup() {
    virtual_gamepad_.reset();
    loading_.reset();
    background_manager_.reset();
    render_sequence_.reset();

    delete_all_cameras();
    delete_all_stages();

    if(sound_driver_) {
        sound_driver_->shutdown();
        sound_driver_.reset();
    }

    delete resource_manager_;
    resource_manager_ = nullptr;

    destroy_window();
    GLThreadCheck::cleanup();
}

void WindowBase::each_stage(std::function<void (uint32_t, Stage*)> func) {
    StageManager::each(func);
}

bool WindowBase::_init() {
    GLThreadCheck::init();

    L_DEBUG("Starting initialization");

#ifdef _arch_dreamcast
    malloc_stats();
#endif

    // Initialize the sound driver (here rather than constructor as it relies on subclass type)
    sound_driver_ = create_sound_driver();
    sound_driver_->startup();

    bool result = create_window(width_, height_, bpp_, fullscreen_);

    // Initialize the render_sequence once we have a renderer
    render_sequence_ = std::make_shared<RenderSequence>(this);

    if(result && !initialized_) {        
        //watcher_ = Watcher::create(*this);

        L_INFO("Registering loaders");
#ifdef _arch_dreamcast
        malloc_stats();
#endif

        //Register the default resource loaders
        register_loader(std::make_shared<smlt::loaders::TextureLoaderType>());
        register_loader(std::make_shared<smlt::loaders::MaterialScriptLoaderType>());
        register_loader(std::make_shared<smlt::loaders::KGLPLoaderType>());
        register_loader(std::make_shared<smlt::loaders::OPTLoaderType>());
        register_loader(std::make_shared<smlt::loaders::OGGLoaderType>());
        register_loader(std::make_shared<smlt::loaders::OBJLoaderType>());
        register_loader(std::make_shared<smlt::loaders::TiledLoaderType>());
        register_loader(std::make_shared<smlt::loaders::HeightmapLoaderType>());
        register_loader(std::make_shared<smlt::loaders::Q2BSPLoaderType>());
        register_loader(std::make_shared<smlt::loaders::WALLoaderType>());
        register_loader(std::make_shared<smlt::loaders::MD2LoaderType>());
        register_loader(std::make_shared<smlt::loaders::PCXLoaderType>());
        register_loader(std::make_shared<smlt::loaders::TTFLoaderType>());

        L_INFO("Initializing the default resources");
#ifdef _arch_dreamcast
        malloc_stats();
#endif

        shared_assets->init();

        create_defaults();

        register_panel(1, std::make_shared<StatsPanel>(this));
        register_panel(2, std::make_shared<PartitionerPanel>(this));

        using std::bind;

        //C++11 lambda awesomeness! input_controller isn't initialized yet
        //so we connect ESCAPE in an idle task
        idle->add_once([=]() {
            //Bind the stop_running method to the ESCAPE key
            input_controller().keyboard().key_pressed_connect(
                smlt::KEYBOARD_CODE_ESCAPE, bind(&WindowBase::stop_running, this)
            );
        });

        initialized_ = true;
    }

    L_DEBUG("Initialization finished");
#ifdef _arch_dreamcast
        malloc_stats();
#endif
    return result;
}

void WindowBase::register_panel(uint8_t function_key, std::shared_ptr<Panel> panel) {
    PanelEntry entry;
    entry.panel = panel;
    entry.keyboard_connection = input_controller_->keyboard().key_pressed_connect((KeyboardCode) (int(KEYBOARD_CODE_F1) + (function_key - 1)), [panel](KeyboardCode sym) {
        if(panel->is_active()) {
            panel->deactivate();
        } else {
            panel->activate();
        }
    });

    panels_[function_key] = entry;
}

void WindowBase::unregister_panel(uint8_t function_key) {
    panels_[function_key].keyboard_connection.disconnect();
    panels_.erase(function_key);
}

void WindowBase::set_logging_level(LoggingLevel level) {
    kazlog::get_logger("/")->set_level((kazlog::LOG_LEVEL) level);
}

void WindowBase::_update_thunk(float dt) {
    if(is_paused()) {
        dt = 0.0; //If the application window is not displayed, don't send a deltatime down
        //it's still accessible through get_deltatime if the user needs it
    }

    background_manager_->update(dt);
    StageManager::_update_thunk(dt);
}

void WindowBase::_fixed_update_thunk(float dt) {
    if(is_paused()) return;
    StageManager::_fixed_update_thunk(dt);
}


void WindowBase::run_update() {
    float dt = time_keeper_->delta_time();

    frame_counter_time_ += dt;
    frame_counter_frames_++;

    if(frame_counter_time_ >= 1.0) {
        stats->set_frames_per_second(frame_counter_frames_);

        frame_time_in_milliseconds_ = 1000.0 / double(frame_counter_frames_);
        frame_counter_frames_ = 0;
        frame_counter_time_ = 0.0;
    }

    _update_thunk(dt);
    signal_update_(dt);

    _late_update_thunk(dt);
    signal_late_update_(dt);
}

void WindowBase::run_fixed_updates() {
    while(time_keeper_->use_fixed_step()) {
        _fixed_update_thunk(time_keeper_->fixed_step()); // Run the fixed updates on controllers
        signal_fixed_update_(time_keeper_->fixed_step()); //Trigger any steps

        stats_.increment_fixed_steps();
    }
}

bool WindowBase::run_frame() {
    signal_frame_started_();

    // Update timers
    time_keeper_->update();

    float dt = time_keeper_->delta_time();

    check_events(); // Check for any window events
    Source::update_source(dt); //Update any playing sounds
    input_controller().update(dt); // Update input devices
    shared_assets->update(dt); // Update animated assets

    run_fixed_updates();
    run_update();

    idle_.execute(); //Execute idle tasks before render

    // Garbage collect resources after idle, but before rendering
    resource_manager_->run_garbage_collection();

    /* Don't run the render sequence if we don't have a context, and don't update the resource
     * manager either because that probably needs a context too! */
    {
        std::lock_guard<std::mutex> rendering_lock(context_lock_);
        if(has_context()) {
            render_sequence()->run();

            signal_pre_swap_();

            swap_buffers();
            GLChecker::end_of_frame_check();

            //std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }

    signal_frame_finished_();

    stats_.increment_frames();

    if(!is_running_) {
        signal_shutdown_();

        loading_.reset();

        //Shutdown the input controller
        input_controller_.reset();

        std::cout << "Frames rendered: " << stats_.frames_run() << std::endl;
        std::cout << "Fixed updates run: " << stats_.fixed_steps_run() << std::endl;
        std::cout << "Total time: " << time_keeper->total_elapsed_seconds() << std::endl;
        std::cout << "Average FPS: " << float(stats_.frames_run()) / time_keeper->total_elapsed_seconds() << std::endl;
    }

    return is_running_;

}

void WindowBase::register_loader(LoaderTypePtr loader) {
    if(std::find(loaders_.begin(), loaders_.end(), loader) != loaders_.end()) {
        throw std::logic_error("Tried to add the same loader twice");
    }

    loaders_.push_back(loader);
}

float WindowBase::aspect_ratio() const {
    assert(width_ > 0);
    assert(height_ > 0);

    return float(width_) / float(height_);
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

void WindowBase::enable_virtual_joypad(VirtualGamepadConfig config, bool flipped) {
    if(virtual_gamepad_) {
        virtual_gamepad_.reset();
    }

    virtual_gamepad_ = VirtualGamepad::create(*this, config);
    input_controller().init_virtual_joypad();

    if(flipped) {
        virtual_gamepad_->flip();
    }
}

void WindowBase::disable_virtual_joypad() {
    /*
     * This is a little hacky, but basically when we disable the virtual gamepad's
     * UI, it triggers a series of events that release any buttons which are currently
     * held down. That inevitably calls has_virtual_gamepad, so if we trigger that whole
     * series just by calling reset() has_virtual_gamepad then returns false and the world ends.
     *
     * So, here we disable the gamepad's pipeline before wiping it out, so those events are triggered
     * while the virtual gamepad still exists. Then the pipeline is deleted by reset()
     */

    if(virtual_gamepad_) {
        virtual_gamepad_->_prepare_deletion();
        virtual_gamepad_.reset();
    }
}


/**
 * @brief WindowBase::reset
 *
 * Destroys all stages and releases all loadables. Then resets the
 * window to its original state.
 */
void WindowBase::reset() {
    idle->execute(); //Execute any idle tasks before we go deleting things

    render_sequence()->delete_all_pipelines();

    CameraManager::destroy_all();
    StageManager::destroy_all();
    background_manager_.reset(new BackgroundManager(this));

    create_defaults();
}

/* PipelineHelperAPIInterface */
PipelinePtr WindowBase::pipeline(PipelineID pid){
    return render_sequence_->pipeline(pid);
}

bool WindowBase::enable_pipeline(PipelineID pid) {
    /*
     * Enables the specified pipeline, returns true if the pipeline
     * was enabled, or false if it was already enabled
     */
    auto pipeline = render_sequence_->pipeline(pid);
    bool state = pipeline->is_active();
    pipeline->activate();
    return state != pipeline->is_active();
}

bool WindowBase::disable_pipeline(PipelineID pid) {
    /*
     * Disables the specified pipeline, returns true if the pipeline
     * was disabled, or false if it was already disabled
     */
    auto pipeline = render_sequence_->pipeline(pid);
    bool state = pipeline->is_active();
    pipeline->deactivate();
    return state != pipeline->is_active();
}

void WindowBase::delete_pipeline(PipelineID pid) {
    render_sequence_->delete_pipeline(pid);
}

bool WindowBase::has_pipeline(PipelineID pid) const {
    return render_sequence_->contains(pid);
}

bool WindowBase::is_pipeline_enabled(PipelineID pid) const {
    return render_sequence_->pipeline(pid)->is_active();
}
/* End PipelineHelperAPIInterface */

void WindowBase::on_key_down(KeyboardCode code, ModifierKeyState modifiers) {
    each_event_listener([=](EventListener* listener) {
        listener->handle_key_down(this, code, modifiers);
    });
}

void WindowBase::on_key_up(KeyboardCode code, ModifierKeyState modifiers) {
    each_event_listener([=](EventListener* listener) {
        listener->handle_key_up(this, code, modifiers);
    });
}

void WindowBase::on_finger_down(TouchPointID touch_id, float normalized_x, float normalized_y, float pressure) {
    each_event_listener([&](EventListener* listener) {
        listener->handle_touch_begin(
            this,
            touch_id,
            normalized_x,
            normalized_y,
            pressure
        );
    });
}

void WindowBase::on_finger_up(TouchPointID touch_id, float normalized_x, float normalized_y) {
    each_event_listener([&](EventListener* listener) {
        listener->handle_touch_end(
            this,
            touch_id,
            normalized_x,
            normalized_y
        );
    });
}

void WindowBase::on_finger_motion(
    TouchPointID touch_id,
    float normalized_x, float normalized_y,
    float dx, float dy // Between -1.0 and +1.0
) {
    each_event_listener([&](EventListener* listener) {
        listener->handle_touch_move(
            this,
            touch_id,
            normalized_x,
            normalized_y,
            dx,
            dy
        );
    });
}

}
