//
//   Copyright (c) 2011-2017 Luke Benstead https://simulant-engine.appspot.com
//
//     This file is part of Simulant.
//
//     Simulant is free software: you can redistribute it and/or modify
//     it under the terms of the GNU Lesser General Public License as published by
//     the Free Software Foundation, either version 3 of the License, or
//     (at your option) any later version.
//
//     Simulant is distributed in the hope that it will be useful,
//     but WITHOUT ANY WARRANTY; without even the implied warranty of
//     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//     GNU Lesser General Public License for more details.
//
//     You should have received a copy of the GNU Lesser General Public License
//     along with Simulant.  If not, see <http://www.gnu.org/licenses/>.
//

#include "scene_manager.h"
#include "scene.h"
#include "../window.h"

namespace smlt {

SceneManager::SceneManager(Window *window):
    window_(window) {

}

SceneManager::~SceneManager() {

}

void SceneManager::destroy_all() {
    /* Unload all the routes */
    for(auto route: routes_) {
        route.second->_call_unload();

        auto name = route.first;
        window_->idle->add_once([this, name]() {
            auto it = routes_.find(name);
            if(it != routes_.end()) {
                routes_.erase(it);
            }
        });
    }
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


void SceneManager::load(const std::string& route) {
    auto scene = get_or_create_route(route);
    scene->_call_load();
}

void SceneManager::load_in_background(const std::string& route, bool redirect_after) {
    auto scene = get_or_create_route(route);

    window_->start_coroutine(
        [=](){
            scene->_call_load();
            if(redirect_after) {
                activate(route);
            }
        }
    );
}

void SceneManager::unload(const std::string& route) {
    auto it = routes_.find(route);
    if(it != routes_.end()) {
        auto scene = it->second;
        scene->_call_unload();
        if(scene->destroy_on_unload()) {
            /* Destroy the scene once it's been unloaded but do
             * it in an idle tasks so that any queued destructions
             * from unload can happen before we destroy the scene
             */

            window_->idle->add_once([this, route, scene]() {
                auto it = routes_.find(route);
                if(it != routes_.end() && it->second == scene) {
                    routes_.erase(it);
                }
            });
        }
    }
}

bool SceneManager::is_loaded(const std::string& route) const {
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
        p.second->_call_unload();
    }
    routes_.clear();
    scene_factories_.clear();
}

bool SceneManager::scene_queued_for_activation() const {
    return scenes_queued_for_activation_ > 0;
}

sig::Connection SceneManager::connect_to_post_idle(std::function<void ()> func) {
    return window_->signal_post_idle().connect(func);
}

}
