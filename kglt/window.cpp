#include <GLee.h>

#include <SDL/SDL.h>
#include "kazbase/unicode.h"
#include "input_controller.h"
#include "window.h"

namespace kglt {

Window::Window() {

}

Window::~Window() {
	SDL_Quit();
}

void Window::set_title(const std::string& title) {
    SDL_WM_SetCaption(title.c_str(), NULL);
}

void Window::show_cursor(bool value) {
	SDL_ShowCursor(value);
}

void Window::cursor_position(int32_t& mouse_x, int32_t& mouse_y) {
	SDL_GetMouseState(&mouse_x, &mouse_y);
}

void Window::check_events() {
    SDL_Event event;

    while(SDL_PollEvent(&event)) {
        input_controller().handle_event(event);

        switch(event.type) {
            case SDL_ACTIVEEVENT:
                break;
            case SDL_VIDEORESIZE:
                break;
            case SDL_QUIT:
                stop_running();
                break;
            default:
                break;
        }
    }
}

bool Window::create_window(int width, int height, int bpp, bool fullscreen) {
//    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
//    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);

    if(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_JOYSTICK) < 0) {
        return false;
    }


    int32_t flags = SDL_OPENGL;
    if(fullscreen) {
        flags |= SDL_FULLSCREEN;
    }

    surface_ = SDL_SetVideoMode(width, height, bpp, flags);

    //Reset the width and height to whatever was actually created

    width = surface_->w;
    height = surface_->h;

    set_width(width);
    set_height(height);
            
    SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);

    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_SWAP_CONTROL, 0);

    SDL_ShowCursor(0);

    assert(surface_);
    assert(GLEE_VERSION_2_1);

    L_DEBUG(unicode("{0} joysicks found").format(SDL_NumJoysticks()).encode());
    for(uint16_t i = 0; i < SDL_NumJoysticks(); i++) {
        L_DEBUG(SDL_JoystickName(i));
    }

    return true;
}

void Window::swap_buffers() {
    SDL_GL_SwapBuffers();
}

}
