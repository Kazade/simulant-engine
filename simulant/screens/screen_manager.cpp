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

#include "screen_manager.h"
#include "screen.h"
#include "../window_base.h"

namespace smlt {

ScreenManager::ScreenManager(WindowBase &window):
    window_(window) {

    step_conn_ = window.signal_step().connect(std::bind(&ScreenManager::step, this, std::placeholders::_1));
}

ScreenManager::~ScreenManager() {
    step_conn_.disconnect();
}

void ScreenManager::step(double dt) {
    if(active_screen()) {
        active_screen()->step(dt);
    }
}

ScreenBase::ptr ScreenManager::get_or_create_route(const std::string& route) {
    auto it = routes_.find(route);
    if(it == routes_.end()) {
        auto factory = screen_factories_.find(route);
        if(factory == screen_factories_.end()) {
            throw std::logic_error("No such route available: " + route);
        }

        routes_[route] = (*factory).second();
        it = routes_.find(route);
    }
    return it->second;
}

void ScreenManager::register_screen(const std::string& route, ScreenFactory factory) {
    screen_factories_[route] = std::bind(factory, std::reference_wrapper<WindowBase>(window_));
}

ScreenBase::ptr ScreenManager::active_screen() const {
    return current_screen_;
}

void ScreenManager::activate_screen(const std::string& route) {
    auto new_screen = get_or_create_route(route);

    if(new_screen == current_screen_) {
        return;
    }

    new_screen->load();

    if(current_screen_) {
        current_screen_->deactivate();
    }

    std::swap(current_screen_, new_screen);
    current_screen_->activate();
}

void ScreenManager::load_screen(const std::string& route) {
    auto screen = get_or_create_route(route);
    screen->load();
}

void ScreenManager::load_screen_in_background(const std::string& route, bool redirect_after) {
    auto screen = get_or_create_route(route);

    //Create a background task for loading the screen
    auto new_task = std::shared_ptr<BackgroundTask>(new BackgroundTask{
        route,
        std::async(std::launch::async, std::bind(&ScreenBase::load, screen))
    });

    // Add an idle task to check for when the background task completes
    window_.idle->add([=]() -> bool {
        // Checks for complete or failed tasks
        auto status = new_task->future.wait_for(std::chrono::microseconds(0));
        if(status != std::future_status::ready) {
            return true; //Try again next frame
        }

        new_task->future.get();
        if(redirect_after) {
            activate_screen(route);
        }

        return false;
    });
}

void ScreenManager::unload_screen(const std::string& route) {
    auto it = routes_.find(route);
    if(it != routes_.end()) {
        it->second->unload();
    }
}

bool ScreenManager::is_screen_loaded(const std::string& route) const {
    auto it = routes_.find(route);
    if(it == routes_.end()) {
        return false;
    } else {
        return it->second->is_loaded();
    }
}

bool ScreenManager::has_screen(const std::string& route) const {
    return screen_factories_.find(route) != screen_factories_.end();
}

ScreenBase::ptr ScreenManager::resolve_screen(const std::string& route) {
    return get_or_create_route(route);
}

void ScreenManager::reset() {
    for(auto p: routes_) {
        p.second->unload();
    }
    routes_.clear();
    screen_factories_.clear();
}

}
