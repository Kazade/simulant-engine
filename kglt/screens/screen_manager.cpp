#include "screen_manager.h"
#include "screen.h"
#include "../window_base.h"

namespace kglt {

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

ScreenBase::ptr ScreenManager::get_or_create_route(const unicode& route) {
    auto it = routes_.find(route);
    if(it == routes_.end()) {
        auto factory = screen_factories_.find(route);
        if(factory == screen_factories_.end()) {
            throw ValueError("No such route available: " + route);
        }

        routes_[route] = (*factory).second();
        it = routes_.find(route);
    }
    return it->second;
}

void ScreenManager::register_screen(const unicode& route, ScreenFactory factory) {
    screen_factories_[route] = std::bind(factory, std::reference_wrapper<WindowBase>(window_));
}

ScreenBase::ptr ScreenManager::active_screen() const {
    return current_screen_;
}

void ScreenManager::activate_screen(const unicode& route) {
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

void ScreenManager::load_screen_in_background(const unicode& route, bool redirect_after) {
    auto screen = get_or_create_route(route);

    //Create a background task for loading the screen
    auto new_task = std::shared_ptr<BackgroundTask>(new BackgroundTask{
        route,
        std::async(std::launch::async, std::bind(&ScreenBase::load, screen))
    });

    // Add an idle task to check for when the background task completes
    window_.idle().add([=]() -> bool {
        // Checks for complete or failed tasks
        auto status = new_task->future.wait_for(std::chrono::microseconds(0));
        if(status != std::future_status::ready) {
            return true; //Try again next frame
        }

        try {
            new_task->future.get();
            if(redirect_after) {
                activate_screen(route);
            }
        } catch(...) {
            L_ERROR("There was an error while loading route: " + route);
            throw;
        }

        return false;
    });
}

void ScreenManager::unload_screen(const unicode& route) {
    auto it = routes_.find(route);
    if(it != routes_.end()) {
        it->second->unload();
    }
}

bool ScreenManager::is_screen_loaded(const unicode& route) const {
    auto it = routes_.find(route);
    if(it == routes_.end()) {
        return false;
    } else {
        return it->second->is_loaded();
    }
}

bool ScreenManager::has_screen(const unicode& route) const {
    return screen_factories_.find(route) != screen_factories_.end();
}

ScreenBase::ptr ScreenManager::resolve_screen(const unicode& route) {
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
