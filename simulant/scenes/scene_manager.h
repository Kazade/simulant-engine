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
#include <future>

#include "../generic/managed.h"
#include "../deps/kazsignal/kazsignal.h"

namespace smlt {

class WindowBase;
class SceneBase;

typedef std::shared_ptr<SceneBase> SceneBasePtr;
typedef std::function<SceneBasePtr (WindowBase&)> SceneFactory;

template<typename T>
SceneFactory scene_factory() {
    SceneFactory ret = &T::template create<WindowBase&>;
    return ret;
}


class SceneManagerInterface {
public:
    virtual ~SceneManagerInterface() {}

    virtual void register_scene(const std::string& route, SceneFactory factory) = 0;
    virtual bool has_scene(const std::string& route) const = 0;
    virtual SceneBasePtr resolve_scene(const std::string& route) = 0;
    virtual void activate_scene(const std::string& route) = 0;
    virtual void load_scene(const std::string& route) = 0;
    virtual void load_scene_in_background(const std::string& route, bool redirect_after=true) = 0;
    virtual void unload_scene(const std::string& route) = 0;
    virtual bool is_scene_loaded(const std::string& route) const = 0;
    virtual SceneBasePtr active_scene() const = 0;
    virtual const std::unordered_map<std::string, SceneBasePtr> routes() const = 0;

    template<typename T>
    std::shared_ptr<T> resolve_scene_as(const std::string& route) {
        return std::dynamic_pointer_cast<T>(resolve_scene(route));
    }

    template<typename T>
    std::shared_ptr<T> active_scene_as() const {
        return std::dynamic_pointer_cast<T>(active_scene());
    }
};


class SceneManager :
    public Managed<SceneManager>,
    public SceneManagerInterface {

public:
    SceneManager(WindowBase& window);
    ~SceneManager();

    void register_scene(const std::string& route, SceneFactory factory);
    bool has_scene(const std::string& route) const;
    SceneBasePtr resolve_scene(const std::string& route);
    void activate_scene(const std::string& route);

    void load_scene(const std::string& route);
    void load_scene_in_background(const std::string& route, bool redirect_after=true);
    void unload_scene(const std::string& route);

    bool is_scene_loaded(const std::string& route) const;
    void reset();
    SceneBasePtr active_scene() const;

    const std::unordered_map<std::string, SceneBasePtr> routes() const {
        return routes_;
    }
private:
    WindowBase& window_;
    std::unordered_map<std::string, std::function<SceneBasePtr ()>> scene_factories_;
    std::unordered_map<std::string, SceneBasePtr> routes_;

    SceneBasePtr current_scene_;

    SceneBasePtr get_or_create_route(const std::string& route);

    struct BackgroundTask {
        std::string route;
        std::future<void> future;
    };

    sig::connection step_conn_;

    void fixed_update(double step);
};

}

#endif // SCENE_MANAGER_H
