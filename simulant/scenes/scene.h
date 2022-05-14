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

#ifndef SCENE_H
#define SCENE_H

/**
 *  Allows you to register different scenes of gameplay, and
 *  easily switch between them.
 *
 *  manager->register_scene<LoadingScene>("loading");
 *  manager->register_scene<MenuScene>("menu");
 *  manager->register_scene<GameScene>("ingame");
 *
 *  manager->activate("loading");
 *  manager->load_in_background("menu");
 *  if(manager->is_loaded("menu")) {
 *      manager->activate("menu");
 *  }
 *  manager->unload("loading");
 *  manager->activate("loading"); // Will cause loading to happen again
 *
 */

#include <set>

#include "../utils/unicode.h"
#include "../types.h"
#include "../generic/managed.h"
#include "../generic/property.h"
#include "../interfaces/nameable.h"
#include "../interfaces/updateable.h"
#include "../interfaces.h"
#include "../stage_manager.h"
#include "../generic/any/any.h"

namespace smlt {

class Application;
class Window;
class InputManager;
class SceneManager;

class SceneLoadException : public std::runtime_error {};

class SceneBase:
    public StageManager {

public:
    typedef std::shared_ptr<SceneBase> ptr;

    SceneBase(Window* window);
    virtual ~SceneBase();

    void _call_load();
    void _call_unload();

    void _call_activate();
    void _call_deactivate();

    bool is_loaded() const { return is_loaded_; }
    bool is_active() const { return is_active_; }

    const std::string name() const {
        return name_;
    }

    void set_name(const std::string& name) {
        name_ = name;
    }

    /* Whether or not to destroy the scene when it's been unloaded.
     * If destroyed, the next time the scene is accessed by name via the scene manager
     * a new instance will be created.
     */
    bool destroy_on_unload() const { return destroy_on_unload_; }
    void set_destroy_on_unload(bool v) {
        destroy_on_unload_ = v;
    }

    /* Whether or not the scene should be unloaded when it's deactivated
     * this is the default behaviour */
    bool unload_on_deactivate() const { return unload_on_deactivate_; }
    void set_unload_on_deactivate(bool v) {
        unload_on_deactivate_ = v;
    }

protected:
    virtual void load() = 0;
    virtual void unload() {}
    virtual void activate() {}
    virtual void deactivate() {}

    /* Linked pipelines activate and deactivate with the scene */
    void link_pipeline(const std::string& name);
    void unlink_pipeline(const std::string& name);
    void link_pipeline(PipelinePtr pipeline);
    void unlink_pipeline(PipelinePtr pipeline);

    void _update_thunk(float dt) override;
    void _fixed_update_thunk(float dt) override;
private:
    std::set<std::string> linked_pipelines_;

    virtual void pre_load() {}
    virtual void post_unload() {}

    bool is_loaded_ = false;
    bool is_active_ = false;
    bool unload_on_deactivate_ = true;    

    std::string name_;

    bool destroy_on_unload_ = true;

    Window* window_;
    InputManager* input_;
    Application* app_;
    SceneManager* scene_manager_ = nullptr;
    Compositor* compositor_ = nullptr;

    friend class SceneManager;

    std::vector<any> load_args;

protected:
    /* Returns the number of arguments passed when loading */
    std::size_t load_arg_count() const;

    template<typename T>
    T get_load_arg(int i) {
        return any_cast<T>(load_args[i]);
    }

public:
    S_DEFINE_PROPERTY(window, &SceneBase::window_);
    S_DEFINE_PROPERTY(app, &SceneBase::app_);
    S_DEFINE_PROPERTY(input, &SceneBase::input_);
    S_DEFINE_PROPERTY(scenes, &SceneBase::scene_manager_);
    S_DEFINE_PROPERTY(compositor, &SceneBase::compositor_);
};

template<typename T>
class Scene : public SceneBase, public RefCounted<T> {
public:
    Scene(Window* window):
        SceneBase(window) {}

    void clean_up() override {
        _call_unload();
    }
};

}

#endif // SCENE_H
