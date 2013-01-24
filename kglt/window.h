#ifndef WINDOW_H_INCLUDED
#define WINDOW_H_INCLUDED

#include "generic/managed.h"
#include "window_base.h"
#include "interpreter.h"

struct SDL_keysym;
struct SDL_Surface;

namespace kglt {

class Window :
    public WindowBase,
    public Managed<Window>,
    public LuaClass<Window> {

public:
    Window(int width=640, int height=480, int bpp=0, bool fullscreen=false);
    virtual ~Window();

    void set_title(const std::string& title);
    void show_cursor(bool value=true);
    void cursor_position(int32_t& mouse_x, int32_t& mouse_y);
    
    bool init() { init_window(); return true; }

    static void do_lua_export(lua_State& state) {
        luabind::module(&state) [
            luabind::class_<Window>("Window")
                .def(luabind::constructor<int, int, int, bool>())
                .def("set_title", &Window::set_title)
                .def("show_cursor", &Window::show_cursor)
        ];
    }

private:
    SDL_Surface* surface_;

    void create_gl_window(int width, int height, int bpp, bool fullscreen);
    void check_events();
    void swap_buffers();

    sigc::signal<void, KeyCode> signal_key_pressed_;
    sigc::signal<void, KeyCode> signal_key_released_;
};

}

#endif // WINDOW_H_INCLUDED
