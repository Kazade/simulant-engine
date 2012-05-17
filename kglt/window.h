#ifndef WINDOW_H_INCLUDED
#define WINDOW_H_INCLUDED

#include "../glee/GLee.h"

#include <SDL/SDL.h>

#include <sigc++/sigc++.h>
#include "window_base.h"

namespace kglt {

class Window : public WindowBase {
public:
    Window(int width=640, int height=480, int bpp=0) {
        if(SDL_Init(SDL_INIT_VIDEO) < 0) {
            throw std::runtime_error("Unable to initialize SDL");
        }

        create_gl_window(width, height, bpp);
    }

    virtual ~Window() {
        SDL_Quit();
    }

    void set_title(const std::string& title);
    void show_cursor(bool value=true);
    void cursor_position(int32_t& mouse_x, int32_t& mouse_y);
    
    sigc::signal<void, SDL_keysym>& signal_key_pressed() { return signal_key_pressed_; }
    sigc::signal<void, SDL_keysym>& signal_key_released() { return signal_key_released_; }
    
private:
    SDL_Surface* surface_;

    void create_gl_window(int width, int height, int bpp);
    void check_events();
    void swap_buffers();

    sigc::signal<void, SDL_keysym> signal_key_pressed_;
    sigc::signal<void, SDL_keysym> signal_key_released_;
};

}

#endif // WINDOW_H_INCLUDED
