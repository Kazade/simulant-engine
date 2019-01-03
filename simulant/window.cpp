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

#include "profiler.h"

#include "utils/gl_error.h"
#include "window.h"
#include "platform.h"
#include "input/input_state.h"

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
#include "loaders/fnt_loader.h"
#include "loaders/dds_texture_loader.h"

#include "nodes/camera.h"

#include "renderers/renderer_config.h"
#include "sound.h"
#include "render_sequence.h"
#include "stage.h"
#include "virtual_gamepad.h"
#include "scenes/loading.h"
#include "utils/gl_thread_check.h"
#include "utils/gl_error.h"
#include "utils/memory.h"

#include "panels/stats_panel.h"
#include "panels/partitioner_panel.h"


/* Icon to send to all screens on boot */

#define simulant_icon_vmu_width 48
#define simulant_icon_vmu_height 32

static unsigned char simulant_icon_vmu_bits[] = {
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x0c, 0x00, 0x00, 0x00, 0x00, 0x80, 0x0b, 0x00,
   0x00, 0x00, 0x00, 0x70, 0x05, 0x00, 0x00, 0x00, 0x00, 0xae, 0x06, 0x00,
   0x00, 0x00, 0xc0, 0x55, 0x03, 0x00, 0x00, 0x00, 0xb8, 0xaa, 0x02, 0x00,
   0x00, 0x00, 0x57, 0x55, 0x01, 0x00, 0x00, 0xc0, 0xaf, 0xaa, 0x01, 0x00,
   0x00, 0x40, 0xfe, 0xd7, 0x00, 0x00, 0x00, 0x80, 0xf8, 0x7f, 0x00, 0x00,
   0x00, 0x00, 0xc1, 0xff, 0x03, 0x00, 0x00, 0x00, 0x02, 0xfe, 0x07, 0x00,
   0x00, 0x00, 0x04, 0xf0, 0x07, 0x00, 0x00, 0x00, 0x0e, 0x80, 0x0f, 0x00,
   0x00, 0x00, 0x1f, 0x00, 0x0c, 0x00, 0x00, 0x00, 0x1f, 0x00, 0x04, 0x00,
   0x00, 0x80, 0x3f, 0x00, 0x03, 0x00, 0x00, 0x80, 0x7f, 0x80, 0x00, 0x00,
   0x00, 0x00, 0xff, 0x40, 0x00, 0x00, 0x00, 0x00, 0xfa, 0x31, 0x00, 0x00,
   0x00, 0x00, 0xd5, 0x0b, 0x00, 0x00, 0x00, 0x80, 0xaa, 0x07, 0x00, 0x00,
   0x00, 0x40, 0x75, 0x00, 0x00, 0x00, 0x00, 0xa0, 0x0f, 0x00, 0x00, 0x00,
   0x00, 0xf0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x18, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };


namespace smlt {

Window::Window(int width, int height, int bpp, bool fullscreen, bool enable_vsync):
    Source(this),
    StageManager(this),
    BackgroundManager(this),
    resource_manager_(new AssetManager(this)),
    initialized_(false),
    width_(-1),
    height_(-1),
    is_running_(true),
    idle_(*this),
    resource_locator_(ResourceLocator::create(this)),
    frame_counter_time_(0),
    frame_counter_frames_(0),
    frame_time_in_milliseconds_(0),
    time_keeper_(TimeKeeper::create(1.0 / Window::STEPS_PER_SECOND)) {

    set_width(width);
    set_height(height);
    set_bpp(bpp);
    set_fullscreen(fullscreen);
    set_vsync_enabled(enable_vsync);

}

Window::~Window() {

}

RenderSequence* Window::render_sequence() {
    return render_sequence_.get();
}

LoaderPtr Window::loader_for(const unicode &filename, LoaderHint hint) {
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


LoaderPtr Window::loader_for(const unicode& loader_name, const unicode &filename) {
    unicode final_file = resource_locator->locate_file(filename);
    
    for(LoaderTypePtr loader_type: loaders_) {
        if(loader_type->name() == loader_name) {
            if(loader_type->supports(final_file)) {
                return loader_type->loader_for(final_file, resource_locator->open_file(final_file));
            } else {
                throw std::logic_error(_u("Loader '{0}' does not support file '{1}'").format(loader_name, filename).encode());
            }
        }
    }

    return LoaderPtr();
}

LoaderTypePtr Window::loader_type(const unicode& loader_name) const {
    for(LoaderTypePtr loader_type: loaders_) {
        if(loader_type->name() == loader_name) {
            return loader_type;
        }
    }
    return LoaderTypePtr();
}

void Window::create_defaults() {
    loading_ = scenes::Loading::create(this);

    //This needs to happen after SDL or whatever is initialized
    input_state_ = InputState::create();
    input_manager_ = InputManager::create(input_state_.get());

    // Tell subclasses to initialize input devices
    initialize_input_controller(*input_state_);
}

void Window::_cleanup() {
    delete_all_backgrounds();

    virtual_gamepad_.reset();
    loading_.reset();
    render_sequence_.reset();

    delete_all_stages();

    if(sound_driver_) {
        sound_driver_->shutdown();
        sound_driver_.reset();
    }

    resource_manager_.reset();

    destroy_window();
    GLThreadCheck::cleanup();
}

void Window::each_stage(std::function<void (uint32_t, Stage*)> func) {
    StageManager::each(func);
}

bool Window::_init() {
    GLThreadCheck::init();

    L_DEBUG("Starting initialization");

#ifdef _arch_dreamcast
    print_available_ram();
#endif

    // Initialize the sound driver (here rather than constructor as it relies on subclass type)
    sound_driver_ = create_sound_driver();
    sound_driver_->startup();

    renderer_ = new_renderer(this, std::getenv("SIMULANT_RENDERER"));

    bool result = create_window();

    // Initialize the render_sequence once we have a renderer
    render_sequence_ = std::make_shared<RenderSequence>(this);

    if(result && !initialized_) {        
        //watcher_ = Watcher::create(*this);

        L_INFO("Registering loaders");
#ifdef _arch_dreamcast
        print_available_ram();
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
        register_loader(std::make_shared<smlt::loaders::FNTLoaderType>());
        register_loader(std::make_shared<smlt::loaders::DDSTextureLoaderType>());

        L_INFO("Initializing the default resources");
#ifdef _arch_dreamcast
        print_available_ram();
#endif

        shared_assets->init();

        create_defaults();

        register_panel(1, std::make_shared<StatsPanel>(this));
        register_panel(2, std::make_shared<PartitionerPanel>(this));

        initialized_ = true;
    }

    L_DEBUG("Initialization finished");
#ifdef _arch_dreamcast
        print_available_ram();
#endif

    idle->add_once([this]() {
        each_screen([](std::string name, Screen* screen) {
            if(screen->width() / screen->integer_scale() == simulant_icon_vmu_width && screen->height() / screen->integer_scale() == simulant_icon_vmu_height) {
                screen->render(simulant_icon_vmu_bits, SCREEN_FORMAT_G1);
            }
        });
    });

    return result;
}

void Window::register_panel(uint8_t function_key, std::shared_ptr<Panel> panel) {
    PanelEntry entry;
    entry.panel = panel;

    panel->set_activation_key((KeyboardCode) (int(KEYBOARD_CODE_F1) + (function_key - 1)));

    panels_[function_key] = entry;
    register_event_listener(panel.get());
}

void Window::unregister_panel(uint8_t function_key) {   
    unregister_event_listener(panels_[function_key].panel.get());
    panels_.erase(function_key);
}

void Window::set_logging_level(LoggingLevel level) {
    kazlog::get_logger("/")->set_level((kazlog::LOG_LEVEL) level);
}

void Window::_update_thunk(float dt) {
    if(is_paused()) {
        dt = 0.0; //If the application window is not displayed, don't send a deltatime down
        //it's still accessible through get_deltatime if the user needs it
    }

    BackgroundManager::update(dt);
    StageManager::_update_thunk(dt);
}

void Window::_fixed_update_thunk(float dt) {
    if(is_paused()) return;
    StageManager::_fixed_update_thunk(dt);
}


void Window::run_update() {
    float dt = time_keeper_->delta_time();

    frame_counter_time_ += dt;
    frame_counter_frames_++;

    if(frame_counter_time_ >= 1.0) {
        stats->set_frames_per_second(frame_counter_frames_);

        frame_time_in_milliseconds_ = 1000.0 / float(frame_counter_frames_);

        stats->set_frame_time(frame_time_in_milliseconds_);

        frame_counter_frames_ = 0;
        frame_counter_time_ = 0.0;
    }

    _update_thunk(dt);
    signal_update_(dt);

    _late_update_thunk(dt);
    signal_late_update_(dt);
}

void Window::run_fixed_updates() {
    while(time_keeper_->use_fixed_step()) {
        _fixed_update_thunk(time_keeper_->fixed_step()); // Run the fixed updates on controllers
        signal_fixed_update_(time_keeper_->fixed_step()); //Trigger any steps

        stats_.increment_fixed_steps();
    }
}

void Window::request_frame_time(float ms) {
    requested_frame_time_ms_ = ms;
}

void Window::await_frame_time() {
    auto this_time = time_keeper_->now_in_us();
    while((float(this_time - last_frame_time_us_) * 0.001f) < requested_frame_time_ms_) {

        platform->sleep_ms(0);

        this_time = time_keeper_->now_in_us();
    }
    last_frame_time_us_ = this_time;
}

Screen* Window::_create_screen(const std::string &name, uint16_t width, uint16_t height, ScreenFormat format, uint16_t refresh_rate) {
    if(screens_.count(name)) {
        L_WARN("Tried to add duplicate Screen");
        return screens_.at(name).get();
    }

    auto screen = Screen::create(this, name);
    screen->width_ = width;
    screen->height_ = height;
    screen->format_ = format;
    screen->refresh_rate_ = refresh_rate;

    if(!initialize_screen(screen.get())) {
        return nullptr;
    }

    screens_.insert(std::make_pair(name, screen));

    signal_screen_added_(name, screen.get());

    return screen.get();
}

void Window::_destroy_screen(const std::string &name) {
    auto screen = screens_.at(name);
    screens_.erase(name);
    signal_screen_removed_(name, screen.get());

    shutdown_screen(screen.get());
}

bool Window::run_frame() {
    static bool first_frame = true;

    await_frame_time(); /* Frame limiter */

    Profiler profiler(__func__);

    signal_frame_started_();

    float dt = 0.0f;
    if(!first_frame) {
        // Update timers
        time_keeper_->update();
        dt = time_keeper_->delta_time();
    }

    check_events(); // Check for any window events

    profiler.checkpoint("event_poll");

    Source::update_source(dt); //Update any playing sounds
    input_state_->update(dt); // Update input devices
    input_manager_->update(dt); // Now update any manager stuff based on the new input state
    shared_assets->update(dt); // Update animated assets

    profiler.checkpoint("asset_updates");

    run_fixed_updates();

    profiler.checkpoint("fixed_updates");

    run_update();

    profiler.checkpoint("updates");

    idle_.execute(); //Execute idle tasks before render

    profiler.checkpoint("idle");

    // Garbage collect resources after idle, but before rendering
    resource_manager_->run_garbage_collection();

    profiler.checkpoint("garbage_collection");

    /* Don't run the render sequence if we don't have a context, and don't update the resource
     * manager either because that probably needs a context too! */
    {
        std::lock_guard<std::mutex> rendering_lock(context_lock_);
        if(has_context()) {

            stats->reset_polygons_rendered();
            render_sequence_->run();

            signal_pre_swap_();

            swap_buffers();
            GLChecker::end_of_frame_check();

            //std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }

    profiler.checkpoint("rendering");

    signal_frame_finished_();

    /* We totally ignore the first frame as it can take a while and messes up
     * delta time for both updates (like particle systems) and FPS
     */
    if(first_frame) {
        first_frame = false;
        time_keeper_->restart();
    } else {
        stats_.increment_frames();
    }

    if(!is_running_) {
        signal_shutdown_();

        loading_.reset();

        //Shutdown the input controller
        input_state_.reset();

        std::cout << "Frames rendered: " << stats_.frames_run() << std::endl;
        std::cout << "Fixed updates run: " << stats_.fixed_steps_run() << std::endl;
        std::cout << "Total time: " << time_keeper->total_elapsed_seconds() << std::endl;
        std::cout << "Average FPS: " << float(stats_.frames_run() - 1) / (time_keeper->total_elapsed_seconds()) << std::endl;

        if(getenv("SIMULANT_PROFILE")) {
            profiler.get_root()->print_stats();
        }
    }

    return is_running_;

}

void Window::register_loader(LoaderTypePtr loader) {
    if(std::find(loaders_.begin(), loaders_.end(), loader) != loaders_.end()) {
        throw std::logic_error("Tried to add the same loader twice");
    }

    loaders_.push_back(loader);
}

float Window::aspect_ratio() const {
    assert(width_ > 0);
    assert(height_ > 0);

    return float(width_) / float(height_);
}

void Window::set_paused(bool value) {
    if(value == is_paused_) return;

    if(value) {
        L_INFO("Pausing application");
    } else {
        L_INFO("Unpausing application");
    }

    is_paused_ = value;
}

void Window::set_has_context(bool value) {
    if(value == has_context_) return;

    has_context_ = value;
}

void Window::enable_virtual_joypad(VirtualGamepadConfig config, bool flipped) {
    if(virtual_gamepad_) {
        virtual_gamepad_.reset();
    }

    virtual_gamepad_ = VirtualGamepad::create(*this, config);
    input_state_->init_virtual_joypad();

    if(flipped) {
        virtual_gamepad_->flip();
    }
}

void Window::disable_virtual_joypad() {
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
 * @brief Window::reset
 *
 * Destroys all stages and releases all loadables. Then resets the
 * window to its original state.
 */
void Window::reset() {
    L_DEBUG("Resetting Window state");

    idle->execute(); //Execute any idle tasks before we go deleting things

    render_sequence_->delete_all_pipelines();

    StageManager::destroy_all();
    background_manager_.reset(new BackgroundManager(this));

    L_DEBUG("Resetting the base manager");
    /* Destroy and recreate the base resource manager */
    resource_manager_.reset();

    L_DEBUG("Reinitializing the base manager");

    resource_manager_.reset(new AssetManager(this));
    assert(resource_manager_);
    resource_manager_->init();

    L_DEBUG("Recreating defaults");
    create_defaults();
}

PipelineHelper Window::render(StagePtr stage, CameraPtr camera) {
    // This is a common enough requirement to provide a nice shortcut
    return render(stage->id(), camera->id());
}

/* PipelineHelperAPIInterface */
PipelinePtr Window::pipeline(PipelineID pid){
    return render_sequence_->pipeline(pid);
}

bool Window::enable_pipeline(PipelineID pid) {
    /*
     * Enables the specified pipeline, returns true if the pipeline
     * was enabled, or false if it was already enabled
     */
    auto pipeline = render_sequence_->pipeline(pid);
    bool state = pipeline->is_active();
    pipeline->activate();
    return state != pipeline->is_active();
}

bool Window::disable_pipeline(PipelineID pid) {
    /*
     * Disables the specified pipeline, returns true if the pipeline
     * was disabled, or false if it was already disabled
     */
    auto pipeline = render_sequence_->pipeline(pid);
    bool state = pipeline->is_active();
    pipeline->deactivate();
    return state != pipeline->is_active();
}

PipelinePtr Window::delete_pipeline(PipelineID pid) {
    render_sequence_->delete_pipeline(pid);
    return nullptr;
}

bool Window::has_pipeline(PipelineID pid) const {
    return render_sequence_->contains(pid);
}

bool Window::is_pipeline_enabled(PipelineID pid) const {
    return render_sequence_->pipeline(pid)->is_active();
}
/* End PipelineHelperAPIInterface */

void Window::on_key_down(KeyboardCode code, ModifierKeyState modifiers) {
    if(code == KEYBOARD_CODE_ESCAPE && escape_to_quit_enabled()) {
        stop_running();
    }

    each_event_listener([=](EventListener* listener) {
        listener->handle_key_down(this, code, modifiers);
    });
}

void Window::on_key_up(KeyboardCode code, ModifierKeyState modifiers) {
    each_event_listener([=](EventListener* listener) {
        listener->handle_key_up(this, code, modifiers);
    });
}

std::size_t Window::screen_count() const {
    return screens_.size();
}

Screen *Window::screen(const std::string &name) const {
    auto it = screens_.find(name);
    if(it != screens_.end()) {
        return it->second.get();
    }

    L_INFO(_F("Unable to find screen with name {0}").format(name));
    return nullptr;
}

void Window::each_screen(std::function<void (std::string, Screen *)> callback) {
    for(auto p: screens_) {
        callback(p.first, p.second.get());
    }
}

void Window::on_finger_down(TouchPointID touch_id, float normalized_x, float normalized_y, float pressure) {
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

void Window::on_finger_up(TouchPointID touch_id, float normalized_x, float normalized_y) {
    each_event_listener([&](EventListener* listener) {
        listener->handle_touch_end(
            this,
            touch_id,
            normalized_x,
            normalized_y
        );
    });
}

void Window::on_finger_motion(
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

void Window::set_audio_listener(StageNode* node) {
    std::lock_guard g(audio_listener_mutex_);

    if(audio_listener_connection_) {
        audio_listener_connection_.disconnect();
    }

    audio_listener_ = node;
    audio_listener_connection_ = audio_listener_->signal_destroyed().connect(
        [this]() {
            audio_listener_ = null;
            audio_listener_connection_.disconnect();
        }
    );
}

StageNode* Window::audio_listener() const {
    std::lock_guard g(audio_listener_mutex_);

    return audio_listener_;
}

}
