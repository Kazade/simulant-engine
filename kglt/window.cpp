#include "../glee/GLee.h"

#include <SDL/SDL.h>

#include "window.h"

namespace kglt {

Window::Window(int width, int height, int bpp) {
	if(SDL_Init(SDL_INIT_VIDEO) < 0) {
		throw std::runtime_error("Unable to initialize SDL");
	}

	create_gl_window(width, height, bpp);
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
        switch(event.type) {
            case SDL_KEYDOWN:
                signal_key_pressed_((KeyCode)event.key.keysym.sym);
                break;
            case SDL_KEYUP:
                signal_key_released_((KeyCode)event.key.keysym.sym);
                break;
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

void Window::create_gl_window(int width, int height, int bpp) {
//    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
//    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);

    set_width(width);
    set_height(height);
        
    surface_ = SDL_SetVideoMode(width, height, bpp, SDL_OPENGL);

    SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 8);

    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);

    assert(surface_);
    assert(GLEE_VERSION_2_1);
}

void Window::swap_buffers() {
    SDL_GL_SwapBuffers();
}

}
