/* *   Copyright (c) 2011-2017 Luke Benstead https://simulant-engine.appspot.com
 *
 *     This file is part of Simulant.
 *
 *     Simulant is free software: you can redistribute it and/or modify
 *     it under the terms of the GNU Lesser General Public License as published by
 *     the Free Software Foundation, either version 3 of the License, or
 *     (at your option) any later version.
 *
 *     Simulant is distributed in the hope that it will be useful,
 *     but WITHOUT ANY WARRANTY; without even the implied warranty of
 *     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *     GNU Lesser General Public License for more details.
 *
 *     You should have received a copy of the GNU Lesser General Public License
 *     along with Simulant.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef SCENE_MANAGER_H
#define SCENE_MANAGER_H

#include <unordered_map>
#include <functional>

#include "../threads/future.h"

#include "scene.h"
#include "../generic/managed.h"
#include "../signals/signal.h"

#include "../generic/static_if.h"
#include "../coroutines/helpers.h"

namespace smlt {

class Application;

typedef std::shared_ptr<Scene> ScenePtr;
typedef std::function<ScenePtr (Window*)> SceneFactory;

typedef sig::signal<void (std::string, Scene*)> SceneActivatedSignal;
typedef sig::signal<void (std::string, Scene*)> SceneDeactivatedSignal;


enum ActivateBehaviour {
    ACTIVATE_BEHAVIOUR_UNLOAD_FIRST,
    ACTIVATE_BEHAVIOUR_UNLOAD_AFTER
};


class SceneManager :
    public RefCounted<SceneManager> {

    friend class Application;

    DEFINE_SIGNAL(SceneActivatedSignal, signal_scene_activated);
    DEFINE_SIGNAL(SceneDeactivatedSignal, signal_scene_deactivated);

    template<typename T>
    static void unpack(std::vector<any>& output, T&& arg) {
        output.push_back(std::forward<T>(arg));
    }

    template<typename T, typename... Args>
    static void unpack(std::vector<any>& output, T&& t, Args&& ...args) {
        output.push_back(std::forward<T>(t));
        unpack(output, std::forward<Args>(args)...);
    }

    template<typename... Args>
    static void unpack(std::vector<any>&) {}

    struct ConnectionHolder {
        sig::connection conn;
    };

    template<typename... Args>
    void do_activate(
        std::string route,
        ActivateBehaviour behaviour,
        Args&&... args
    ) {
        auto new_scene = get_or_create_route(route);
        if(new_scene != current_scene_) {
            if(behaviour == ACTIVATE_BEHAVIOUR_UNLOAD_AFTER) {
                if(!new_scene->is_loaded()) {
                    new_scene->load_args.clear();
                    unpack(new_scene->load_args, std::forward<Args>(args)...);
                    new_scene->_call_load();
                }

                auto previous = current_scene_;

                if(previous) {
                    previous->_call_deactivate();
                    signal_scene_deactivated_(previous->name(), previous.get());
                }

                std::swap(current_scene_, new_scene);
                current_scene_->_call_activate();

                if(previous && previous->unload_on_deactivate()) {
                    // If requested, we unload the previous scene once the new on is active
                    unload(previous->name());
                }
            } else {
                /* Default behaviour - unload the current scene before activating the next one */
                auto previous = current_scene_;

                if(previous) {
                    previous->_call_deactivate();
                    signal_scene_deactivated_(previous->name(), previous.get());
                    if(previous->unload_on_deactivate()) {
                        unload(previous->name());
                    }
                }

                if(!new_scene->is_loaded()) {
                    new_scene->load_args.clear();
                    unpack(new_scene->load_args, std::forward<Args>(args)...);
                    new_scene->_call_load();
                }

                std::swap(current_scene_, new_scene);
                current_scene_->_call_activate();
            }

            signal_scene_activated_(route, new_scene.get());
        }
    }
public:
    SceneManager(Window* window);
    ~SceneManager();

    bool has_scene(const std::string& route) const;
    ScenePtr resolve_scene(const std::string& route);

    template<typename... Args>
    void activate(
        const std::string& route, ActivateBehaviour behaviour,
        Args&& ...args
    ) {
        scene_activation_trigger_ = std::bind(
            &SceneManager::do_activate<Args&...>,
            this, route, behaviour, std::forward<Args>(args)...
        );
    }

    template<typename... Args>
    void activate(const std::string& route, Args&& ...args) {
        activate(route, ACTIVATE_BEHAVIOUR_UNLOAD_FIRST, std::forward<Args>(args)...);
    }

    template<typename ...Args>
    void preload(const std::string& route, Args&& ...args) {
        auto scene = get_or_create_route(route);

        if(scene->is_loaded()) {
            return;
        }

        scene->load_args.clear();
        unpack(scene->load_args, std::forward<Args>(args)...);

        scene->_call_load();
    }

    template<typename ...Args>
    void _preload_in_background(ScenePtr scene, Args&& ...args) {
        if(scene->is_loaded()) {
            return;
        }

        scene->load_args.clear();
        unpack(scene->load_args, std::forward<Args>(args)...);
        scene->_call_load();
    }

    template<typename ...Args>
    Promise<void> preload_in_background(
        const std::string& route,
        Args&& ...args
    ) {

        auto scene = get_or_create_route(route);

        return cr_async(
            std::bind(
                &SceneManager::_preload_in_background<Args&...>,
                this,
                scene,
                std::forward<Args>(args)...
            )
        );
    }

    void unload(const std::string& route);

    /* Unloads and destroys all scenes */
    void destroy_all();

    bool is_loaded(const std::string& route) const;
    void reset();

    ScenePtr active_scene() const;

    bool scene_queued_for_activation() const;

    template<typename T, typename... Args>
    void register_scene(const std::string& name, Args&&... args) {
        SceneFactory func = std::bind(
            &T::template create<Window*, typename std::decay<Args>::type&...>,
            std::placeholders::_1, std::forward<Args>(args)...
        );

        _store_scene_factory(name, [=](Window* window) -> ScenePtr {
            auto ret = func(window);
            ret->set_name(name);
            ret->scene_manager_ = this;
            return ret;
        });
    }

    template<typename T>
    void register_scene(const std::string& name) {
        _store_scene_factory(name, [=](Window* window) -> ScenePtr {
            auto ret = T::create(window);
            ret->set_name(name);
            ret->scene_manager_ = this;
            return ret;
        });
    }

    template<typename T>
    std::shared_ptr<T> resolve_scene_as(const std::string& route) {
        return std::dynamic_pointer_cast<T>(resolve_scene(route));
    }
private:
    void _store_scene_factory(const std::string& name, SceneFactory func) {
        scene_factories_[name] = func;
    }

    Window* window_;

    std::unordered_map<std::string, SceneFactory> scene_factories_;
    std::unordered_map<std::string, ScenePtr> routes_;

    ScenePtr current_scene_;
    ScenePtr get_or_create_route(const std::string& route);

    struct BackgroundTask {
        std::string route;
        thread::Future<void> future;
    };

    sig::connection step_conn_;
    sig::connection update_conn_;
    sig::connection late_update_conn_;

    void update(float dt);
    void late_update(float dt);
    void fixed_update(float step);

    std::function<void ()> scene_activation_trigger_;
    std::set<std::string> routes_queued_for_destruction_;
};

}

#endif // SCENE_MANAGER_H
