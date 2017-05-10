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

#include "scene_manager.h"
#include "scene.h"
#include "../window_base.h"

namespace smlt {

SceneManager::SceneManager(WindowBase *window):
    window_(window) {

    step_conn_ = window_->signal_fixed_update().connect(std::bind(&SceneManager::fixed_update, this, std::placeholders::_1));
    update_conn_ = window->signal_update().connect(std::bind(&SceneManager::update, this, std::placeholders::_1));
    late_update_conn_ = window->signal_late_update().connect(std::bind(&SceneManager::late_update, this, std::placeholders::_1));
}

SceneManager::~SceneManager() {
    step_conn_.disconnect();
    update_conn_.disconnect();
    late_update_conn_.disconnect();
}

void SceneManager::late_update(float dt) {
    if(active_scene()) {
        active_scene()->_late_update_thunk(dt);
    }
}

void SceneManager::update(float dt) {
    if(active_scene()) {
        active_scene()->_update_thunk(dt);
    }
}

void SceneManager::fixed_update(float dt) {
    if(active_scene()) {
        active_scene()->_fixed_update_thunk(dt);
    }
}

SceneBase::ptr SceneManager::get_or_create_route(const std::string& route) {
    auto it = routes_.find(route);
    if(it == routes_.end()) {
        auto factory = scene_factories_.find(route);
        if(factory == scene_factories_.end()) {
            throw std::logic_error("No such route available: " + route);
        }

        routes_[route] = (*factory).second(window_);
        it = routes_.find(route);
    }
    return it->second;
}

SceneBase::ptr SceneManager::active_scene() const {
    return current_scene_;
}

void SceneManager::activate_scene(const std::string& route) {
    auto new_scene = get_or_create_route(route);

    if(new_scene == current_scene_) {
        return;
    }

    new_scene->load();

    if(current_scene_) {
        current_scene_->deactivate();
    }

    std::swap(current_scene_, new_scene);
    current_scene_->activate();
}

void SceneManager::load_scene(const std::string& route) {
    auto scene = get_or_create_route(route);
    scene->load();
}

void SceneManager::load_scene_in_background(const std::string& route, bool redirect_after) {
    auto scene = get_or_create_route(route);

    //Create a background task for loading the scene
    auto new_task = std::shared_ptr<BackgroundTask>(new BackgroundTask{
        route,
#ifdef _arch_dreamcast
        stdX::async(std::bind(&SceneBase::load, scene))
#else
        std::async(std::launch::async, std::bind(&SceneBase::load, scene))
#endif
    });

    // Add an idle task to check for when the background task completes
    window_->idle->add([=]() -> bool {
        // Checks for complete or failed tasks
        auto status = new_task->future.wait_for(std::chrono::microseconds(0));
#ifdef _arch_dreamcast
        if(status != stdX::future_status::ready) {
            return true;
        }
#else
        if(status != std::future_status::ready) {
            return true; //Try again next frame
        }
#endif
        new_task->future.get();
        if(redirect_after) {
            activate_scene(route);
        }

        return false;
    });
}

void SceneManager::unload_scene(const std::string& route) {
    auto it = routes_.find(route);
    if(it != routes_.end()) {
        it->second->unload();
    }
}

bool SceneManager::is_scene_loaded(const std::string& route) const {
    auto it = routes_.find(route);
    if(it == routes_.end()) {
        return false;
    } else {
        return it->second->is_loaded();
    }
}

bool SceneManager::has_scene(const std::string& route) const {
    return scene_factories_.find(route) != scene_factories_.end();
}

SceneBase::ptr SceneManager::resolve_scene(const std::string& route) {
    return get_or_create_route(route);
}

void SceneManager::reset() {
    for(auto p: routes_) {
        p.second->unload();
    }
    routes_.clear();
    scene_factories_.clear();
}

}
