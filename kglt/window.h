#ifndef WINDOW_H_INCLUDED
#define WINDOW_H_INCLUDED

#include "window_base.h"

struct SDL_keysym;
struct SDL_Surface;

namespace kglt {

class Window : public WindowBase {
public:
    typedef std::tr1::shared_ptr<Window> ptr;


    virtual ~Window();

    void set_title(const std::string& title);
    void show_cursor(bool value=true);
    void cursor_position(int32_t& mouse_x, int32_t& mouse_y);
    
    sigc::signal<void, KeyCode>& signal_key_down() { return signal_key_pressed_; }
    sigc::signal<void, KeyCode>& signal_key_up() { return signal_key_released_; }
    
    static kglt::Window::ptr create(int width=640, int height=480, int bpp=0) {
        kglt::Window::ptr new_window(new kglt::Window(width, height, bpp));
        new_window->init();
        return new_window;
    }

private:
    Window(int width=640, int height=480, int bpp=0);

    SDL_Surface* surface_;

    void create_gl_window(int width, int height, int bpp);
    void check_events();
    void swap_buffers();

    sigc::signal<void, KeyCode> signal_key_pressed_;
    sigc::signal<void, KeyCode> signal_key_released_;
};

}

#endif // WINDOW_H_INCLUDED
