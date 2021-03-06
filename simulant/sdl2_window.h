/* *   Copyright (c) 2011-2017 Luke Benstead https://simulant-engine.appspot.com
 *
 *     This file is part of Simulant.
 *
 *     Simulant is free software: you can redistribute it and/or modify
 *     it under the terms of the GNU Lesser General Public License as published by
 *     the Free Software Foundation, either version 3 of the License, or
 *     (at your option) any later version.
 *
 *     Simulant is distributed in the hope that it will be useful,
 *     but WITHOUT ANY WARRANTY; without even the implied warranty of
 *     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *     GNU Lesser General Public License for more details.
 *
 *     You should have received a copy of the GNU Lesser General Public License
 *     along with Simulant.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef SIMULANT_WINDOW_H_INCLUDED
#define SIMULANT_WINDOW_H_INCLUDED

#include <string>

#include <SDL.h>
#if defined(__WIN32__)
    #undef main
#endif
#include "sdl2_keycodes.h"
#include "generic/managed.h"
#include "window.h"
#include "platform.h"

namespace smlt {

int event_filter(void* user_data, SDL_Event* event);

class SDL2Window : public Window {

    class SDLPlatform : public Platform {
    public:
        std::string name() const override {
#if defined(__LINUX__)
            return "linux";
#elif defined(__APPLE__)
            return "darwin";
#elif defined(__ANDROID__)
            return "android";
#elif defined(__WIN32__)
            return "windows";
#else
    #error Currently unsupported platform
#endif
        }

        Resolution native_resolution() const override {
            SDL_DisplayMode mode;

            Resolution native;
            if(SDL_GetDesktopDisplayMode(0, &mode) == -1) {
                S_WARN("Unable to get the current desktop display mode!!");
                S_WARN("{0}", SDL_GetError());
                S_WARN("Falling back to 1080p");
                native.width = 1920;
                native.height = 1080;
                native.refresh_rate = 60;
            } else {
                native.width = mode.w;
                native.height = mode.h;
                native.refresh_rate = mode.refresh_rate;
            }
            return native;
        }

        uint64_t available_ram_in_bytes() const override;
        uint64_t total_ram_in_bytes() const override;
        uint64_t available_vram_in_bytes() const override {
            return MEMORY_VALUE_UNAVAILABLE;
        }

        uint64_t process_ram_usage_in_bytes(ProcessID process_id) const override;
    };

public:
    static const SDLPlatform platform;

    static Window::ptr create(Application* app) {
        return Window::create<SDL2Window>(app);
    }

    SDL2Window();
    virtual ~SDL2Window();

    void set_title(const std::string& title) override;
    void show_cursor(bool value=true) override;
    void lock_cursor(bool cursor_locked=true) override;
    void cursor_position(int32_t& mouse_x, int32_t& mouse_y) override;

private:
    bool _init_window() override;
    bool _init_renderer(Renderer* renderer) override;

    SDL_Window* screen_;
    SDL_GLContext context_;

    void destroy_window() override;

    void check_events() override;
    void swap_buffers() override;

    friend int event_filter(void* user_data, SDL_Event* event);

    void denormalize(float x, float y, int& xout, int& yout);

    std::shared_ptr<SoundDriver> create_sound_driver(const std::string &from_config) override;

    void initialize_input_controller(InputState &controller) override;

    /* This is for testing mainly. If you set config.desktop.enable_virtual_screen = true */
    void initialize_virtual_screen(uint16_t width, uint16_t height, ScreenFormat format, uint16_t integer_scale);

    std::vector<SDL_Joystick*> open_joysticks_;
    std::vector<SDL_GameController*> open_controllers_;

    bool initialize_screen(Screen *screen) override;
    void shutdown_screen(Screen* screen) override;
    void render_screen(Screen* screen, const uint8_t* data) override;

    friend class Application;
};

}

#endif // WINDOW_H_INCLUDED
