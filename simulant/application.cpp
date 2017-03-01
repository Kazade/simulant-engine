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

#include "sdl2_window.h"
#include "application.h"
#include "screens/loading.h"
#include "input_controller.h"

namespace smlt {

Application::Application(const AppConfig &config):
    config_(config) {
    construct_window(config);
}

Application::Application(const unicode &title, uint32_t width, uint32_t height, uint32_t bpp, bool fullscreen){
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
    window_ = SDL2Window::create(this, config.width, config.height, config.bpp, config.fullscreen);

    for(auto& search_path: config.search_paths) {
        window_->resource_locator->add_search_path(search_path);
    }

    if(!window_->_init()) {
        throw InstanceInitializationError("Unable to create window");
    }

    routes_.reset(new ScreenManager(*window_));

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
    // Add some useful screens by default, these can be overridden in do_init if the
    // user so wishes
    register_screen("/loading", screen_factory<screens::Loading>());
    load_screen("/loading");

    initialized_ = do_init();

    // If we successfully initialized, but the user didn't specify
    // a particular screen, we just hit the root route
    if(initialized_ && !active_screen()) {
        activate_screen("/");
    }

    return initialized_;
}


int32_t Application::run() {
    if(!init()) {
        L_ERROR("Error while initializing, terminating application");
        return 1;
    }

    while(window_->run_frame()) {}

    // Shutdown any screens
    routes_.reset();

    // Shutdown and clean up the window
    window_->_cleanup();
    window_.reset();

    return 0;
}

}
