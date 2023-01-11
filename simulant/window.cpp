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
#include "nodes/camera.h"
#include "nodes/ui/ui_manager.h"
#include "asset_manager.h"
#include "renderers/renderer_config.h"
#include "compositor.h"
#include "scenes/loading.h"
#include "utils/gl_thread_check.h"
#include "utils/gl_error.h"

#include "panels/stats_panel.h"
#include "panels/partitioner_panel.h"

#include "vfs.h"


namespace smlt {

Window::Window():
    initialized_(false),
    width_(-1),
    height_(-1) {


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

    has_focus_ = true;

    update_conn_ = application->signal_late_update().connect(std::bind(&Window::update_screens, this, std::placeholders::_1));

    return true;
}

void Window::create_defaults() {
    //This needs to happen after SDL or whatever is initialized
    input_state_ = InputState::create(this);
    input_manager_ = InputManager::create(input_state_.get());

    // Tell subclasses to initialize input devices
    initialize_input_controller(*input_state_);
}

void Window::_clean_up() {
    update_conn_.disconnect();
    auto screens = screens_;
    for(auto& screen: screens) {
        _destroy_screen(screen.first);
    }

    auto panels = panels_;
    for(auto p: panels) {
        unregister_panel(p.first);
    }
    panels.clear();

    compositor_.reset();

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

    // Initialize the render_sequence once we have a renderer
    compositor_ = std::make_shared<Compositor>(this);

    if(!initialized_) {
        /* Swap buffers immediately after creation, this makes sure that
         * on platforms like the Dreamcast we definitely clear to black before
         * spending time loading anything */
        swap_buffers();

        //watcher_ = Watcher::create(*this);

        S_INFO("Initializing the default resources");

        create_defaults();
        create_panels();

        initialized_ = true;
    }

    S_DEBUG("Initialization finished");

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

Screen* Window::_create_screen(const std::string &name, uint16_t width, uint16_t height, ScreenFormat format, uint16_t refresh_rate) {
    if(screens_.count(name)) {
        S_WARN("Tried to add duplicate Screen");
        return screens_.at(name).get();
    }

    auto screen = Screen::create(this, name, width, height, format, refresh_rate);

    if(!initialize_screen(screen.get())) {
        return nullptr;
    }

    screens_.insert(std::make_pair(name, screen));

    signal_screen_added_(name, screen.get());

    S_INFO("Created screen: {0}", name);
    return screen.get();
}

void Window::_destroy_screen(const std::string &name) {
    S_INFO("Destroyed screen: {0}", name);
    auto screen = screens_.at(name);
    screens_.erase(name);
    signal_screen_removed_(name, screen.get());

    shutdown_screen(screen.get());
}

float Window::aspect_ratio() const {
    assert(width_ > 0);
    assert(height_ > 0);

    return float(width_) / float(height_);
}

void Window::update_screens(float dt) {
    for(auto& scr: screens_) {
        scr.second->update(dt);
    }
}

void Window::set_has_context(bool value) {
    if(value == has_context_) return;

    has_context_ = value;
}

void Window::create_panels() {
    destroy_panels();

    S_DEBUG("Recreating panels");
    register_panel(1, StatsPanel::create(this));
    // register_panel(2, PartitionerPanel::create(this));
}

void Window::destroy_panels() {
    auto panels = panels_;
    for(auto p: panels) {
        unregister_panel(p.first);
    }
    panels.clear();
}

/**
 * @brief Window::reset
 *
 * Destroys all everying and resets the
 * window to its original state.
 */
void Window::reset() {
    S_DEBUG("Resetting Window state");

    // Make sure we aren't still getting text input
    input->stop_text_input();

    compositor_->destroy_all_pipelines();
    compositor_->clean_up();

    S_DEBUG("Recreating defaults");
    create_defaults();
    create_panels();
}

void Window::on_key_down(KeyboardCode code, ModifierKeyState modifiers) {
    if(code == KEYBOARD_CODE_ESCAPE && escape_to_quit_enabled()) {
        application->stop_running();
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

void Window::on_game_controller_button_down(GameControllerIndex index, JoystickButton button) {
    each_event_listener([=](EventListener* listener) {
        listener->handle_controller_button_down(index, button);
    });
}

void Window::on_game_controller_button_up(GameControllerIndex index, JoystickButton button) {
    each_event_listener([=](EventListener* listener) {
        listener->handle_controller_button_up(index, button);
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
    for(auto& p: screens_) {
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

}
