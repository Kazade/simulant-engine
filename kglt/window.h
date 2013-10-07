#ifndef KGLT_WINDOW_H_INCLUDED
#define KGLT_WINDOW_H_INCLUDED

#include <SDL.h>

#include "generic/managed.h"
#include "window_base.h"

namespace kglt {

class Window :
    public WindowBase {

public:
    static WindowBase::ptr create(int width=640, int height=480, int bpp=0, bool fullscreen=false) {
        return WindowBase::create<Window>(width, height, bpp, fullscreen);
    }

    Window();
    virtual ~Window();

    void set_title(const std::string& title);
    void show_cursor(bool value=true);
    void cursor_position(int32_t& mouse_x, int32_t& mouse_y);
    
    sig::signal<void (SDL_Scancode)>& signal_key_down() { return signal_key_pressed_; }
    sig::signal<void (SDL_Scancode)>& signal_key_up() { return signal_key_released_; }
    
private:
    SDL_Window* screen_;
    SDL_GLContext context_;

    bool create_window(int width, int height, int bpp, bool fullscreen);
    void check_events();
    void swap_buffers();

    sig::signal<void (SDL_Scancode)> signal_key_pressed_;
    sig::signal<void (SDL_Scancode)> signal_key_released_;
};

}

#endif // WINDOW_H_INCLUDED
