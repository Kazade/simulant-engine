//
//   Copyright (c) 2011-2017 Luke Benstead https://simulant-engine.appspot.com
//
//     This file is part of Simulant.
//
//     Simulant is free software: you can redistribute it and/or modify
//     it under the terms of the GNU General Public License as published by
//     the Free Software Foundation, either version 3 of the License, or
//     (at your option) any later version.
//
//     Simulant is distributed in the hope that it will be useful,
//     but WITHOUT ANY WARRANTY; without even the implied warranty of
//     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//     GNU General Public License for more details.
//
//     You should have received a copy of the GNU General Public License
//     along with Simulant.  If not, see <http://www.gnu.org/licenses/>.
//

#include <chrono>
#include <future>

#ifdef _arch_dreamcast
#include "kos_window.h"
namespace smlt { typedef KOSWindow Window; }
#else
#include "sdl2_window.h"
namespace smlt { typedef SDL2Window Window; }
#endif

#include "application.h"
#include "scenes/loading.h"
#include "input_controller.h"

namespace smlt {

Application::Application(const AppConfig &config):
    config_(config) {

    construct_window(config);
}

Application::Application(const unicode &title, uint32_t width, uint32_t height, uint32_t bpp, bool fullscreen) {

    AppConfig config;
    config.title = title;;
    config.width = width;
    config.height = height;
    config.bpp = bpp;
    config.fullscreen = fullscreen;

    config_ = config;
    construct_window(config);
}

void Application::construct_window(const AppConfig& config) {

    kazlog::get_logger("/")->add_handler(kazlog::Handler::ptr(new kazlog::StdIOHandler));
    L_DEBUG("Constructing the window");

    window_ = Window::create(this, config.width, config.height, config.bpp, config.fullscreen);

    for(auto& search_path: config.search_paths) {
        window_->resource_locator->add_search_path(search_path);
    }

    L_DEBUG("Search paths added successfully");

    if(!window_->_init()) {
        throw InstanceInitializationError("Unable to create window");
    }

    window_->set_title(config.title.encode());

    window_->signal_fixed_update().connect(std::bind(&Application::do_fixed_update, this, std::placeholders::_1));
    window_->signal_shutdown().connect(std::bind(&Application::do_cleanup, this));

    window_->keyboard->key_pressed_connect(std::bind(&Application::on_key_press, this, std::placeholders::_1));
    window_->keyboard->key_released_connect(std::bind(&Application::on_key_release, this, std::placeholders::_1));
    window_->keyboard->key_while_pressed_connect(std::bind(&Application::while_key_pressed, this, std::placeholders::_1, std::placeholders::_2));
}

StagePtr Application::stage(StageID stage) {
    return window->stage(stage);
}

bool Application::init() {
    L_DEBUG("Initializing the application");

    scene_manager_.reset(new SceneManager(window_.get()));

    // Add some useful scenes by default, these can be overridden in do_init if the
    // user so wishes
    register_scene<scenes::Loading>("_loading");
    load_scene("_loading");

    initialized_ = do_init();

    // If we successfully initialized, but the user didn't specify
    // a particular scene, we just hit the root route
    if(initialized_ && !active_scene()) {
        activate_scene("main");
    }

    return initialized_;
}


int32_t Application::run() {
    if(!init()) {
        L_ERROR("Error while initializing, terminating application");
        return 1;
    }

    while(window_->run_frame()) {}

    // Reset the scene manager (destroying scenes) before the window
    // disappears
    scene_manager_.reset();

    // Shutdown and clean up the window
    window_->_cleanup();
    window_.reset();

    return 0;
}

bool Application::has_scene(const std::string &route) const {
    return scene_manager_->has_scene(route);
}

SceneBasePtr Application::resolve_scene(const std::string &route) {
    return scene_manager_->resolve_scene(route);
}

void Application::activate_scene(const std::string &route) {
    scene_manager_->activate_scene(route);
}

void Application::load_scene(const std::string &route) {
    scene_manager_->load_scene(route);
}

void Application::load_scene_in_background(const std::string &route, bool redirect_after) {
    scene_manager_->load_scene_in_background(route);
}

void Application::unload_scene(const std::string &route) {
    scene_manager_->unload_scene(route);
}

bool Application::is_scene_loaded(const std::string &route) const {
    return scene_manager_->is_scene_loaded(route);
}

void Application::reset() {
    scene_manager_->reset();
}

SceneBasePtr Application::active_scene() const {
    return scene_manager_->active_scene();
}

void Application::_store_scene_factory(const std::string &name, std::function<SceneBasePtr (WindowBase *)> func) {
    scene_manager_->_store_scene_factory(name, func);
}

}
