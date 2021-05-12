//
//   Copyright (c) 2011-2017 Luke Benstead https://simulant-engine.appspot.com
//
//     This file is part of Simulant.
//
//     Simulant is free software: you can redistribute it and/or modify
//     it under the terms of the GNU Lesser General Public License as published by
//     the Free Software Foundation, either version 3 of the License, or
//     (at your option) any later version.
//
//     Simulant is distributed in the hope that it will be useful,
//     but WITHOUT ANY WARRANTY; without even the implied warranty of
//     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//     GNU Lesser General Public License for more details.
//
//     You should have received a copy of the GNU Lesser General Public License
//     along with Simulant.  If not, see <http://www.gnu.org/licenses/>.
//

#ifdef ___DREAMCAST__
    #include <kos.h>
#endif

#include "application.h"
#include "utils/gl_error.h"
#include "window.h"
#include "platform.h"
#include "input/input_state.h"

#include "loaders/texture_loader.h"
#include "loaders/material_script.h"
#include "loaders/opt_loader.h"
#include "loaders/ogg_loader.h"
#include "loaders/obj_loader.h"
#include "loaders/particle_script.h"
#include "loaders/heightmap_loader.h"
#include "loaders/q2bsp_loader.h"
#include "loaders/wal_loader.h"
#include "loaders/md2_loader.h"
#include "loaders/pcx_loader.h"
#include "loaders/ttf_loader.h"
#include "loaders/fnt_loader.h"
#include "loaders/dds_texture_loader.h"
#include "loaders/wav_loader.h"
#include "loaders/ms3d_loader.h"
#include "loaders/dtex_loader.h"

#include "nodes/camera.h"

#include "renderers/renderer_config.h"
#include "sound.h"
#include "compositor.h"
#include "stage.h"
#include "virtual_gamepad.h"
#include "scenes/loading.h"
#include "utils/gl_thread_check.h"
#include "utils/gl_error.h"

#include "panels/stats_panel.h"
#include "panels/partitioner_panel.h"
#include "stage_manager.h"


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

Window::Window():
    Source(this),
    StageManager(this),
    asset_manager_(new SharedAssetManager(this)),
    initialized_(false),
    width_(-1),
    height_(-1),
    is_running_(true),
    idle_(*this),
    vfs_(VirtualFileSystem::create(this)),
    frame_counter_time_(0),
    frame_counter_frames_(0),
    frame_time_in_milliseconds_(0),
    time_keeper_(TimeKeeper::create(1.0 / Window::STEPS_PER_SECOND)) {


}

Window::~Window() {

}

bool Window::create_window(uint16_t width, uint16_t height, uint8_t bpp, bool fullscreen, bool enable_vsync) {
    set_width(width);
    set_height(height);
    set_bpp(bpp);
    set_fullscreen(fullscreen);
    set_vsync_enabled(enable_vsync);

    GLThreadCheck::init();

    _init_window();

    renderer_ = new_renderer(
        this,
        application_->config_.development.force_renderer
    );

    _init_renderer(renderer_.get());

    renderer_->init_context();

    return true;
}

LoaderPtr Window::loader_for(const Path &filename, LoaderHint hint) {

    Path final_file;
    try {
        final_file = vfs->locate_file(filename);
    } catch(AssetMissingError&) {
        S_ERROR("Couldn't get loader as file doesn't exist");
        return LoaderPtr();
    }

    std::vector<std::pair<LoaderTypePtr, LoaderPtr>> possible_loaders;

    for(LoaderTypePtr loader_type: loaders_) {
        if(loader_type->supports(final_file)) {
            S_DEBUG("Found possible loader: {0}", loader_type->name());
            auto new_loader = loader_type->loader_for(final_file, vfs->open_file(final_file));
            new_loader->set_vfs(this->vfs_.get());

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

        throw std::logic_error(_F("More than one possible loader was found for '{0}'. Please specify a hint.").format(filename));
    }

    S_WARN("No suitable loader found for {0}", filename);
    return LoaderPtr();
}


LoaderPtr Window::loader_for(const std::string& loader_name, const Path& filename) {
    Path final_file = vfs->locate_file(filename);

    for(LoaderTypePtr loader_type: loaders_) {
        if(loader_type->name() == loader_name) {
            if(loader_type->supports(final_file)) {
                S_DEBUG("Found loader {0} for file: {1}", loader_name, filename.str());
                return loader_type->loader_for(final_file, vfs->open_file(final_file));
            } else {
                S_ERROR("Loader '{0}' does not support file '{1}'", loader_name, filename);
                return LoaderPtr();
            }
        }
    }

    S_ERROR("Unable to find loader for: {0}", filename.str());
    return LoaderPtr();
}

LoaderTypePtr Window::loader_type(const std::string& loader_name) const {
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

void Window::_clean_up() {
    virtual_gamepad_.reset();
    loading_.reset();

    auto panels = panels_;
    for(auto p: panels) {
        unregister_panel(p.first);
    }
    panels.clear();

    destroy_all_stages();
    StageManager::clean_up();

    compositor_.reset();

    if(sound_driver_) {
        sound_driver_->shutdown();
        sound_driver_.reset();
    }

    asset_manager_.reset();

    destroy_window();
    GLThreadCheck::clean_up();
}

StageNode* Window::audio_listener()  {
    if(audio_listener_) {
        return audio_listener_;
    } else {
        // Return the first camera we're going to render with
        for(auto pip: *compositor_) {
            if(pip->is_active()) {
                return pip->camera();
            }
        }

        return nullptr;
    }
}

void Window::set_audio_listener(StageNode* node) {
    audio_listener_ = node;
    audio_listener_->signal_destroyed().connect([this]() {
        audio_listener_ = nullptr;
    });
}

bool Window::has_explicit_audio_listener() const {
    return audio_listener_ != nullptr;
}

bool Window::initialize_assets_and_devices() {
    S_DEBUG("Starting initialization");

    // Initialize the sound driver (here rather than constructor as it relies on subclass type)
    sound_driver_ = create_sound_driver(application_->config_.development.force_sound_driver);
    sound_driver_->startup();

    // Initialize the render_sequence once we have a renderer
    compositor_ = std::make_shared<Compositor>(this);

    if(!initialized_) {
        /* Swap buffers immediately after creation, this makes sure that
         * on platforms like the Dreamcast we definitely clear to black before
         * spending time loading anything */
        swap_buffers();

        //watcher_ = Watcher::create(*this);

        S_INFO("Registering loaders");

        //Register the default resource loaders
        register_loader(std::make_shared<smlt::loaders::TextureLoaderType>());
        register_loader(std::make_shared<smlt::loaders::MaterialScriptLoaderType>());
        register_loader(std::make_shared<smlt::loaders::ParticleScriptLoaderType>());
        register_loader(std::make_shared<smlt::loaders::OPTLoaderType>());
        register_loader(std::make_shared<smlt::loaders::OGGLoaderType>());
        register_loader(std::make_shared<smlt::loaders::OBJLoaderType>());
        register_loader(std::make_shared<smlt::loaders::HeightmapLoaderType>());
        register_loader(std::make_shared<smlt::loaders::Q2BSPLoaderType>());
        register_loader(std::make_shared<smlt::loaders::WALLoaderType>());
        register_loader(std::make_shared<smlt::loaders::MD2LoaderType>());
        register_loader(std::make_shared<smlt::loaders::PCXLoaderType>());
        register_loader(std::make_shared<smlt::loaders::TTFLoaderType>());
        register_loader(std::make_shared<smlt::loaders::FNTLoaderType>());
        register_loader(std::make_shared<smlt::loaders::DDSTextureLoaderType>());
        register_loader(std::make_shared<smlt::loaders::WAVLoaderType>());
        register_loader(std::make_shared<smlt::loaders::MS3DLoaderType>());
        register_loader(std::make_shared<smlt::loaders::DTEXLoaderType>());

        S_INFO("Initializing the default resources");

        shared_assets->init();

        create_defaults();

        register_panel(1, StatsPanel::create(this));
        register_panel(2, PartitionerPanel::create(this));

        initialized_ = true;
    }

    S_DEBUG("Initialization finished");

    idle->add_once([this]() {
        each_screen([](std::string, Screen* screen) {
            if(screen->width() / screen->integer_scale() == simulant_icon_vmu_width && screen->height() / screen->integer_scale() == simulant_icon_vmu_height) {
                screen->render(simulant_icon_vmu_bits, SCREEN_FORMAT_G1);
            }
        });
    });

    return true;
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

void Window::toggle_panel(uint8_t id) {
    if(panels_[id].panel->is_active()) {
        panels_[id].panel->deactivate();
    } else {
        panels_[id].panel->activate();
    }
}

void Window::activate_panel(uint8_t id) {
    panels_[id].panel->activate();
}

void Window::deactivate_panel(uint8_t id) {
    panels_[id].panel->deactivate();
}

bool Window::panel_is_active(uint8_t id) {
    return panels_[id].panel->is_active();
}

void Window::set_logging_level(LogLevel level) {
    smlt::get_logger("/")->set_level(level);
}

void Window::_update_thunk(float dt) {
    if(is_paused()) {
        dt = 0.0; //If the application window is not displayed, don't send a deltatime down
        //it's still accessible through get_deltatime if the user needs it
    }

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

    if(frame_counter_time_ >= 1.0f) {
        stats->set_frames_per_second(frame_counter_frames_);

        frame_time_in_milliseconds_ = 1000.0f / float(frame_counter_frames_);

        stats->set_frame_time(frame_time_in_milliseconds_);

        frame_counter_frames_ = 0;
        frame_counter_time_ = 0.0f;
    }

    auto sm = application_->scene_manager_;

    _update_thunk(dt);
    if(sm) {
        sm->update(dt);
    }
    signal_update_(dt);

    _late_update_thunk(dt);
    if(sm) {
        sm->late_update(dt);
    }
    signal_late_update_(dt);
}

void Window::run_fixed_updates() {
    while(time_keeper_->use_fixed_step()) {
        float step = time_keeper_->fixed_step();
        _fixed_update_thunk(step); // Run the fixed updates on controllers

        /* Call the scene managed fixed update. FIXME: This is a code
         * smell. The whole Application -> Window/Engine thing needs a bit
         * of a rethink */
        auto sm = application_->scene_manager_;
        if(sm) {
            sm->fixed_update(step);
        }

        signal_fixed_update_(step); //Trigger any steps

        stats_.increment_fixed_steps();
    }
}

void Window::request_frame_time(float ms) {
    requested_frame_time_ms_ = ms;
}

void Window::await_frame_time() {
    if(requested_frame_time_ms_ == 0) {
        return;
    }

    auto this_time = time_keeper_->now_in_us();
    while((float(this_time - last_frame_time_us_) * 0.001f) < requested_frame_time_ms_) {
        thread::sleep(0);
        this_time = time_keeper_->now_in_us();
    }
    last_frame_time_us_ = this_time;
}

Screen* Window::_create_screen(const std::string &name, uint16_t width, uint16_t height, ScreenFormat format, uint16_t refresh_rate) {
    if(screens_.count(name)) {
        S_WARN("Tried to add duplicate Screen");
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

    signal_frame_started_();

    float dt = 0.0f;
    if(!first_frame) {
        // Update timers
        time_keeper_->update();
        dt = time_keeper_->delta_time();
    }

    input_state_->pre_update(dt);
    check_events(); // Check for any window events

    auto listener = audio_listener();
    if(listener) {
        sound_driver_->set_listener_properties(
            listener->absolute_position(),
            listener->absolute_rotation(),
            smlt::Vec3() // FIXME: Where do we get velocity?
        );
    }

    Source::update_source(dt); //Update any playing sounds
    input_state_->update(dt); // Update input devices
    input_manager_->update(dt); // Now update any manager stuff based on the new input state
    shared_assets->update(dt); // Update animated assets

    run_fixed_updates();
    run_update();

    update_idle_tasks_and_coroutines();

    // Garbage collect resources after idle, but before rendering
    asset_manager_->run_garbage_collection();

    StageManager::clean_up();

    /* Don't run the render sequence if we don't have a context, and don't update the resource
     * manager either because that probably needs a context too! */
    {
        thread::Lock<thread::Mutex> rendering_lock(context_lock_);
        if(has_context()) {

            stats->reset_polygons_rendered();
            compositor_->run();

            signal_pre_swap_();

            swap_buffers();
            GLChecker::end_of_frame_check();

            //std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }

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
        S_INFO("Pausing application");
    } else {
        S_INFO("Unpausing application");
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
    S_DEBUG("Resetting Window state");

    idle->execute(); //Execute any idle tasks before we go deleting things

    disable_virtual_joypad();

    auto panels = panels_;
    for(auto p: panels) {
        unregister_panel(p.first);
    }
    panels.clear();

    compositor_->destroy_all_pipelines();
    compositor_->clean_up();

    StageManager::destroy_all_stages();
    StageManager::clean_up();

    S_DEBUG("Resetting the base manager");
    /* Destroy and recreate the base resource manager */
    asset_manager_.reset();

    S_DEBUG("Reinitializing the base manager");

    asset_manager_ = SharedAssetManager::create(this);
    assert(asset_manager_);
    asset_manager_->init();

    S_DEBUG("Recreating defaults");
    create_defaults();

    S_DEBUG("Recreating panels");
    register_panel(1, StatsPanel::create(this));
    register_panel(2, PartitionerPanel::create(this));
}

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

    S_INFO("Unable to find screen with name {0}", name);
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

void Window::start_coroutine(std::function<void ()> func) {
    coroutines_.push_back(cort::start_coroutine(func));
}

void Window::update_idle_tasks_and_coroutines() {
    idle_.execute();
    update_coroutines();
    signal_post_idle_();
}

void Window::update_coroutines() {
    for(auto it = coroutines_.begin(); it != coroutines_.end();) {
        if(cort::resume_coroutine(*it) == cort::CO_RESULT_FINISHED) {
            cort::stop_coroutine(*it);
            it = coroutines_.erase(it);
        } else {
            ++it;
        }
    }
}

void Window::stop_all_coroutines() {
    for(auto it = coroutines_.begin(); it != coroutines_.end(); ++it) {
        cort::stop_coroutine(*it);
    }
}

}
