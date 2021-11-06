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

typedef std::shared_ptr<SceneBase> SceneBasePtr;
typedef std::function<SceneBasePtr (Window*)> SceneFactory;

typedef sig::signal<void (std::string, SceneBase*)> SceneActivatedSignal;

class SceneManager :
    public RefCounted<SceneManager> {

    friend class Application;

    DEFINE_SIGNAL(SceneActivatedSignal, signal_scene_activated);

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
    static void do_activate(
        std::weak_ptr<SceneManager> _this,
        std::shared_ptr<ConnectionHolder> holder,
        const std::string& route,
        Args&&... args
    ) {

        auto self = _this.lock();

        /* Little bit of cleverness to check that the scene manager is still alive */
        if(!self) {
            S_DEBUG("Not activating {0} as SceneManager was destroyed", route);
            holder->conn.disconnect();
            return;
        }

        auto new_scene = self->get_or_create_route(route);
        if(new_scene != self->current_scene_) {
            if(!new_scene->is_loaded()) {
                new_scene->load_args.clear();
                unpack(new_scene->load_args, std::forward<Args>(args)...);
                new_scene->_call_load();
            }

            auto previous = self->current_scene_;

            if(previous) {
                previous->_call_deactivate();
            }

            std::swap(self->current_scene_, new_scene);
            self->current_scene_->_call_activate();

            if(previous && previous->unload_on_deactivate()) {
                // If requested, we unload the previous scene once the new on is active
                self->unload(previous->name());
            }

            self->signal_scene_activated_(route, new_scene.get());
        }

        holder->conn.disconnect();
        self->scenes_queued_for_activation_--;
        assert(self->scenes_queued_for_activation_ >= 0);
    };
public:
    SceneManager(Window* window);
    ~SceneManager();

    bool has_scene(const std::string& route) const;
    SceneBasePtr resolve_scene(const std::string& route);

    template<typename... Args>
    void activate(
        const std::string& route,
        Args&& ...args
    ) {

        auto holder = std::make_shared<ConnectionHolder>();
        std::weak_ptr<SceneManager> _this = shared_from_this();

        /* Little bit of trickery here. We want to activate the scene after idle tasks
         * have run, but then we want to immediately disconnect. So we pass the connection
         * wrapped in a shared_ptr which has been bound to the lambda */
        holder->conn = connect_to_post_idle(
            std::bind(
                SceneManager::do_activate<Args&...>,
                shared_from_this(), holder, route, std::forward<Args>(args)...
            )
        );
        scenes_queued_for_activation_++;
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
    void _preload_in_background(SceneBasePtr scene, Args&& ...args) {
        if(scene->is_loaded()) {
            return;
        }

        scene->load_args.clear();
        unpack(scene->load_args, std::forward<Args>(args)...);
        scene->_call_load();
    }

    template<typename ...Args>
    CRPromise<void> preload_in_background(
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
    /* This exists to avoid a circular include */
    sig::Connection connect_to_post_idle(std::function<void ()> func);

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
