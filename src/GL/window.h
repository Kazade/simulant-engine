#ifndef WINDOW_H_INCLUDED
#define WINDOW_H_INCLUDED

#include <SDL/SDL.h>
#include <SDL/SDL_opengl.h>

#include "scene.h"

namespace GL {

class Window {
public:
    Window(int width=640, int height=480, int bpp=0):
        is_running_(true),
        width_(width),
        height_(height) {

        if(SDL_Init(SDL_INIT_VIDEO) < 0) {
            throw std::runtime_error("Unable to initialize SDL");
        }

        create_gl_window(width, height, bpp);
    }

    ~Window() {
        SDL_Quit();
    }

    void set_title(const std::string& title);
    Scene& scene() { return scene_; }

    bool update();

private:
    bool is_running_;

    SDL_Surface* surface_;

    Scene scene_;

    void create_gl_window(int width, int height, int bpp);
    void check_events();

    uint32_t width_;
    uint32_t height_;
};

}

#endif // WINDOW_H_INCLUDED
