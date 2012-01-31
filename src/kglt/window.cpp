#include "window.h"

namespace kglt {

bool Window::update() {
    check_events();

    //FIXME: Add a Viewport class for this shizzle
    glViewport(0, 0, width_, height_);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);

    glClearColor(0.5, 0.5, 0.5, 1.0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    scene_.render();

    SDL_GL_SwapBuffers();

    return is_running_;
}

void Window::set_title(const std::string& title) {
    SDL_WM_SetCaption(title.c_str(), NULL);
}

void Window::check_events() {
    SDL_Event event;

    while(SDL_PollEvent(&event)) {
        switch(event.type) {
            case SDL_KEYDOWN:
                signal_key_pressed_(event.key.keysym);
                break;
            case SDL_KEYUP:
                signal_key_released_(event.key.keysym);
                break;
            case SDL_ACTIVEEVENT:
                break;
            case SDL_VIDEORESIZE:
                break;
            case SDL_QUIT:
                is_running_ = false;
                break;
            default:
                break;
        }
    }
}

void Window::create_gl_window(int width, int height, int bpp) {
//    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
//    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);

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

void Window::register_loader(LoaderType::ptr loader) {
    //FIXME: assert doesn't exist already
    loaders_.push_back(loader);
}

}
