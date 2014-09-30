#include <kazbase/logging.h>
#include "utils/glcompat.h"
#include "utils/gl_error.h"
#include "kazbase/unicode.h"
#include "input_controller.h"
#include "window.h"

namespace kglt {

Window::Window() {

}

Window::~Window() {
    SDL_GL_DeleteContext(context_);
}

void Window::set_title(const std::string& title) {
    if(screen_) {
        SDL_SetWindowTitle(screen_, title.c_str());
    }
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

    if(SDL_Init(SDL_INIT_EVERYTHING) != 0) {
        L_ERROR(_u("Unable to initialize SDL {0}").format(SDL_GetError()));
        return false;
    }


    int32_t flags = SDL_WINDOW_OPENGL;
    if(fullscreen) {
        flags |= SDL_WINDOW_FULLSCREEN_DESKTOP;
    }

    screen_ = SDL_CreateWindow(
        "",
        SDL_WINDOWPOS_UNDEFINED,
        SDL_WINDOWPOS_UNDEFINED,
        width, height,
        flags
    );
    assert(screen_);

#ifndef __ANDROID__
    //OpenGL 3.1 baby!
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG);
#endif

    SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);

    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);

    context_ = SDL_GL_CreateContext(screen_);
    check_and_log_error(__FILE__, __LINE__);

#ifndef __ANDROID__
    glewExperimental = true;
    GLenum err = glewInit();
    glGetError(); //Sigh, sets this regardless
    assert(err == GLEW_OK);
    assert(GLEW_VERSION_3_1);
#endif
    //Reset the width and height to whatever was actually created
    SDL_GetWindowSize(screen_, &width, &height);

    set_width(width);
    set_height(height);

    SDL_ShowCursor(0);

    L_DEBUG(unicode("{0} joysicks found").format(SDL_NumJoysticks()).encode());
    for(uint16_t i = 0; i < SDL_NumJoysticks(); i++) {
        if(SDL_IsGameController(i)) {
            SDL_GameController* controller = SDL_GameControllerOpen(i);
            L_DEBUG(SDL_GameControllerName(controller));
        }
    }

    check_and_log_error(__FILE__, __LINE__);
    return true;
}

void Window::swap_buffers() {
    SDL_GL_SwapWindow(screen_);
}

}
