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

namespace smlt {

class Application;
class Window;

typedef std::shared_ptr<SceneBase> SceneBasePtr;
typedef std::function<SceneBasePtr (Window*)> SceneFactory;


enum SceneChangeBehaviour {
    SCENE_CHANGE_BEHAVIOUR_UNLOAD_CURRENT_SCENE,
    SCENE_CHANGE_BEHAVIOUR_RETAIN_CURRENT_SCENE
};

class SceneManager :
    public RefCounted<SceneManager> {

    friend class Window;

public:
    SceneManager(Window* window);
    ~SceneManager();

    bool has_scene(const std::string& route) const;
    SceneBasePtr resolve_scene(const std::string& route);
    void activate(const std::string& route, SceneChangeBehaviour behaviour=SCENE_CHANGE_BEHAVIOUR_UNLOAD_CURRENT_SCENE);

    void load(const std::string& route);
    void load_in_background(const std::string& route, bool redirect_after=true);
    void unload(const std::string& route);

    /* Unloads and destroys all scenes */
    void destroy_all();

    bool is_loaded(const std::string& route) const;
    void reset();

    SceneBasePtr active_scene() const;
    bool scene_queued_for_activation() const;

    template<typename T, typename... Args>
    void register_scene(const std::string& name, Args&&... args) {
        SceneFactory func = std::bind(
            &T::template create<Window*, typename std::decay<Args>::type&...>,
            std::placeholders::_1, std::forward<Args>(args)...
        );

        _store_scene_factory(name, [=](Window* window) -> SceneBasePtr {
            auto ret = func(window);
            ret->set_name(name);
            ret->scene_manager_ = this;
            return ret;
        });
    }

    template<typename T>
    void register_scene(const std::string& name) {
        _store_scene_factory(name, [=](Window* window) -> SceneBasePtr {
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
    std::unordered_map<std::string, SceneBasePtr> routes_;

    SceneBasePtr current_scene_;
    int scenes_queued_for_activation_ = 0;


    SceneBasePtr get_or_create_route(const std::string& route);

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
};

}

#endif // SCENE_MANAGER_H
