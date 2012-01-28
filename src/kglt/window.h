#ifndef WINDOW_H_INCLUDED
#define WINDOW_H_INCLUDED

#include "glee/GLee.h"

#include <SDL/SDL.h>

#include <sigc++/sigc++.h>

#include "scene.h"

#include "loaders/texture_loader.h"
#include "loaders/q2bsp_loader.h"
#include "loader.h"

namespace kglt {

class Window {
public:
    Window(int width=640, int height=480, int bpp=0):
        is_running_(true),
        scene_(this),
        width_(width),
        height_(height) {

        if(SDL_Init(SDL_INIT_VIDEO) < 0) {
            throw std::runtime_error("Unable to initialize SDL");
        }

        create_gl_window(width, height, bpp);

        //Register the default resource loaders
        register_loader(LoaderType::ptr(new kglt::loaders::TextureLoaderType));
        register_loader(LoaderType::ptr(new kglt::loaders::Q2BSPLoaderType));
    }

    ~Window() {
        SDL_Quit();
    }

    void set_title(const std::string& title);
    Scene& scene() { return scene_; }
    Loader::ptr loader_for(const std::string& filename) {
        std::string final_file = find_file(filename);

        for(LoaderType::ptr loader_type: loaders_) {
            if(loader_type->supports(final_file)) {
                return loader_type->loader_for(final_file);
            }
        }

        throw std::runtime_error("Unable to find a load for: + filename");
    }

    bool update();

    void register_loader(LoaderType::ptr loader_type);
    void add_search_path(const std::string& path) {
        resource_paths_.push_back(path);
    }

    sigc::signal<void, SDL_keysym>& signal_key_pressed() { return signal_key_pressed_; }
    sigc::signal<void, SDL_keysym>& signal_key_released() { return signal_key_released_; }

private:
    std::string find_file(const std::string& filename) {
        //FIXME: Search the resource paths!
        return filename;
    }

    bool is_running_;

    SDL_Surface* surface_;

    Scene scene_;

    void create_gl_window(int width, int height, int bpp);
    void check_events();

    uint32_t width_;
    uint32_t height_;

    std::vector<std::string> resource_paths_;
    std::vector<LoaderType::ptr> loaders_;

    sigc::signal<void, SDL_keysym> signal_key_pressed_;
    sigc::signal<void, SDL_keysym> signal_key_released_;
};

}

#endif // WINDOW_H_INCLUDED
