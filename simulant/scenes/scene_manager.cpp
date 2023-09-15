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
#include "../application.h"

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
        routes_queued_for_destruction_.insert(name);
    }
}

void SceneManager::late_update(float dt) {
    if(!get_app()->window->has_focus()) {
        // if paused, send deltatime as 0.0.
        // it's still accessible through window->time_keeper if the user needs it
        dt = 0.0;
    }

    if(active_scene()) {
        active_scene()->late_update(dt);

        /* Anything destroyed must now be *really* destroyed */
        active_scene()->clean_up_destroyed_objects();
    }

    /* Destroy any scenes that have been queued */
    for(auto& name: routes_queued_for_destruction_) {
        auto it = routes_.find(name);
        if(it != routes_.end()) {
            routes_.erase(it);
        }
    }

    routes_queued_for_destruction_.clear();

    /* Finally, if a scene has been activated, do so! */
    if(scene_activation_trigger_) {
        scene_activation_trigger_();
        scene_activation_trigger_ = std::function<void()> ();
    }
}

void SceneManager::update(float dt) {
    if(!get_app()->window->has_focus()) {
        // if paused, send deltatime as 0.0.
        // it's still accessible through window->time_keeper if the user needs it
        dt = 0.0;
    }

    if(active_scene()) {
        active_scene()->update(dt);
    }
}

void SceneManager::fixed_update(float dt) {
    if(!get_app()->window->has_focus()) {
        // if paused, send deltatime as 0.0.
        // it's still accessible through window->time_keeper if the user needs it
        dt = 0.0;
    }

    if(active_scene()) {
        active_scene()->fixed_update(dt);
    }
}

ScenePtr SceneManager::get_or_create_route(const std::string& route) {
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

ScenePtr SceneManager::active_scene() const {
    return current_scene_;
}

bool SceneManager::scene_queued_for_activation() const {
    return bool(scene_activation_trigger_);
}

void SceneManager::unload(const std::string& route) {
    auto it = routes_.find(route);
    if(it != routes_.end()) {
        std::shared_ptr<Scene> scene = it->second;
        scene->_call_unload();
        scene->load_args.clear();

        if(scene->destroy_on_unload()) {
            /* Destroy the scene once it's been unloaded but do
             * it after late_update so that any queued destructions
             * from unload can happen before we destroy the scene
             */
            routes_queued_for_destruction_.insert(route);
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

ScenePtr SceneManager::resolve_scene(const std::string& route) {
    return get_or_create_route(route);
}

void SceneManager::reset() {
    for(auto p: routes_) {
        p.second->_call_unload();
    }
    routes_.clear();
    scene_factories_.clear();
}

}
