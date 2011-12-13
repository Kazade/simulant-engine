#ifndef WINDOW_H_INCLUDED
#define WINDOW_H_INCLUDED

#include <SDL/SDL.h>
#include <SDL/SDL_opengl.h>

#include "scene.h"

#include "loaders/texture_loader.h"
#include "loaders/q2bsp_loader.h"

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

        //Register the default resource loaders
        register_loader(LoaderType::ptr(new GL::loaders::TextureLoaderType));
        register_loader(LoaderType::ptr(new GL::loaders::Q2BSPLoaderType));
    }

    ~Window() {
        SDL_Quit();
    }

    void set_title(const std::string& title);
    Scene& scene() { return scene_; }
    Loader loader(const std::string& filename) {
        for(LoaderType::ptr l: loaders) {
            if(l->supports(filename)) {
                return l->loader_for(filename);
            }
        }
    
        throw std::runtime_error("Unable to find a load for: + filename);
    }

    bool update();

    void register_loader(LoaderType::ptr loader_type);

private:
    bool is_running_;

    SDL_Surface* surface_;

    Scene scene_;

    void create_gl_window(int width, int height, int bpp);
    void check_events();

    uint32_t width_;
    uint32_t height_;

    std::vector<LoaderType::ptr> loaders_;

};

}

#endif // WINDOW_H_INCLUDED
