/* *   Copyright (c) 2011-2017 Luke Benstead https://simulant-engine.appspot.com
 *
 *     This file is part of Simulant.
 *
 *     Simulant is free software: you can redistribute it and/or modify
 *     it under the terms of the GNU General Public License as published by
 *     the Free Software Foundation, either version 3 of the License, or
 *     (at your option) any later version.
 *
 *     Simulant is distributed in the hope that it will be useful,
 *     but WITHOUT ANY WARRANTY; without even the implied warranty of
 *     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *     GNU General Public License for more details.
 *
 *     You should have received a copy of the GNU General Public License
 *     along with Simulant.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef SCENE_MANAGER_H
#define SCENE_MANAGER_H

#include <unordered_map>
#include <functional>

#ifdef _arch_dreamcast
#include "../generic/simple_future.h"
#else
#include <future>
#endif

#include "scene.h"

#include "../generic/managed.h"
#include "../deps/kazsignal/kazsignal.h"

namespace smlt {

class Application;
class Window;

typedef std::shared_ptr<SceneBase> SceneBasePtr;
typedef std::function<SceneBasePtr (Window*)> SceneFactory;

class SceneManagerInterface {
public:
    virtual ~SceneManagerInterface() {}

    template<typename T, typename... Args>
    void register_scene(const std::string& name, Args&&... args) {
        SceneFactory func = std::bind(
            &T::template create<Window*, typename std::decay<Args>::type&...>,
            std::placeholders::_1, std::forward<Args>(args)...
        );

        _store_scene_factory(name, [=](Window* window) -> SceneBasePtr {
            auto ret = func(window);
            ret->set_name(name);
            return ret;
        });
    }

    template<typename T>
    void register_scene(const std::string& name) {
        _store_scene_factory(name, [=](Window* window) -> SceneBasePtr {
            auto ret = T::create(window);
            ret->set_name(name);
            return ret;
        });
    }

    template<typename T>
    std::shared_ptr<T> resolve_scene_as(const std::string& route) {
        return std::dynamic_pointer_cast<T>(resolve_scene(route));
    }

    virtual bool has_scene(const std::string& route) const = 0;
    virtual SceneBasePtr resolve_scene(const std::string& route) = 0;
    virtual void activate_scene(const std::string& route) = 0;

    virtual void load_scene(const std::string& route) = 0;
    virtual void load_scene_in_background(const std::string& route, bool redirect_after=true) = 0;
    virtual void unload_scene(const std::string& route) = 0;

    virtual bool is_scene_loaded(const std::string& route) const = 0;
    virtual void reset() = 0;
    virtual SceneBasePtr active_scene() const = 0;

    virtual void _store_scene_factory(const std::string& name, SceneFactory func) = 0;

};

class SceneManager :
    public Managed<SceneManager>,
    public SceneManagerInterface {

public:
    SceneManager(Window* window);
    ~SceneManager();

    bool has_scene(const std::string& route) const;
    SceneBasePtr resolve_scene(const std::string& route);
    void activate_scene(const std::string& route);

    void load_scene(const std::string& route);
    void load_scene_in_background(const std::string& route, bool redirect_after=true);
    void unload_scene(const std::string& route);

    bool is_scene_loaded(const std::string& route) const;
    void reset();
    SceneBasePtr active_scene() const;

    void _store_scene_factory(const std::string& name, SceneFactory func) {
        scene_factories_[name] = func;
    }

private:
    Window* window_;

    std::unordered_map<std::string, SceneFactory> scene_factories_;
    std::unordered_map<std::string, SceneBasePtr> routes_;

    SceneBasePtr current_scene_;

    SceneBasePtr get_or_create_route(const std::string& route);

    struct BackgroundTask {
        std::string route;
#ifdef _arch_dreamcast
        stdX::future<void> future;
#else
        std::future<void> future;
#endif
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
