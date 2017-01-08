/* *   Copyright (c) 2011-2017 Luke Benstead https://simulant-engine.appspot.com
 *
 *     This file is part of Simulant.
 *
 *     Simulant is free software: you can redistribute it and/or modify
 *     it under the terms of the GNU General Public License as published by
 *     the Free Software Foundation, either version 3 of the License, or
 *     (at your option) any later version.
 *
 *     Simulant is distributed in the hope that it will be useful,
 *     but WITHOUT ANY WARRANTY; without even the implied warranty of
 *     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *     GNU General Public License for more details.
 *
 *     You should have received a copy of the GNU General Public License
 *     along with Simulant.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef SIMULANT_WINDOW_H_INCLUDED
#define SIMULANT_WINDOW_H_INCLUDED

#include <SDL.h>

#include "generic/managed.h"
#include "window_base.h"

namespace smlt {

int event_filter(void* user_data, SDL_Event* event);

class SDL2Window :
    public WindowBase {

public:
    static WindowBase::ptr create(Application* app, int width=640, int height=480, int bpp=0, bool fullscreen=false) {
        return WindowBase::create<SDL2Window>(app, width, height, bpp, fullscreen);
    }

    SDL2Window();
    virtual ~SDL2Window();

    void set_title(const std::string& title);
    void show_cursor(bool value=true);
    void cursor_position(int32_t& mouse_x, int32_t& mouse_y);
    
    sig::signal<void (SDL_Scancode)>& signal_key_down() { return signal_key_pressed_; }
    sig::signal<void (SDL_Scancode)>& signal_key_up() { return signal_key_released_; }
    
private:
    SDL_Window* screen_;
    SDL_GLContext context_;

    bool create_window(int width, int height, int bpp, bool fullscreen);
    void destroy_window();

    void check_events();
    void swap_buffers();

    sig::signal<void (SDL_Scancode)> signal_key_pressed_;
    sig::signal<void (SDL_Scancode)> signal_key_released_;

    friend int event_filter(void* user_data, SDL_Event* event);

    void denormalize(float x, float y, int& xout, int& yout);
};

}

#endif // WINDOW_H_INCLUDED
