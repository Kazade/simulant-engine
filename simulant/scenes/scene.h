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

#include "../asset_manager.h"
#include "../compositor.h"
#include "../generic/any/any.h"
#include "../generic/managed.h"
#include "../generic/property.h"
#include "../interfaces.h"
#include "../interfaces/nameable.h"
#include "../interfaces/updateable.h"
#include "../nodes/stage_node_manager.h"
#include "../types.h"
#include "../utils/unicode.h"
#include "simulant/utils/construction_args.h"

namespace smlt {

class Application;
class Window;
class InputManager;
class SceneManager;
class Service;

class SceneLoadException : public std::runtime_error {};

typedef sig::signal<void ()> SceneOnActivatedSignal;
typedef sig::signal<void ()> SceneOnDeactivatedSignal;

typedef sig::signal<void (StageNode*, StageNodeType)> StageNodeCreatedSignal;
typedef sig::signal<void (StageNode*, StageNodeType)> StageNodeDestroyedSignal;

typedef sig::signal<void (Camera*, Viewport*, StageNode*)> LayerRenderStartedSignal;
typedef sig::signal<void (Camera*, Viewport*, StageNode*)> LayerRenderFinishedSignal;


class LightingSettings {
public:
    smlt::Color ambient_light() const {
        return ambient_light_;
    }

    void set_ambient_light(const smlt::Color& c) {
        ambient_light_ = c;
    }

private:
    smlt::Color ambient_light_;
};


class Scene:
    public StageNode,
    public StageNodeManager {

    DEFINE_SIGNAL(SceneOnActivatedSignal, signal_activated);
    DEFINE_SIGNAL(SceneOnDeactivatedSignal, signal_deactivated);

    DEFINE_SIGNAL(StageNodeCreatedSignal, signal_stage_node_created);
    DEFINE_SIGNAL(StageNodeDestroyedSignal, signal_stage_node_destroyed);

    DEFINE_SIGNAL(LayerRenderStartedSignal, signal_layer_render_started);
    DEFINE_SIGNAL(LayerRenderFinishedSignal, signal_layer_render_finished);
public:
    typedef std::shared_ptr<Scene> ptr;

    Scene(Window* window);
    virtual ~Scene();

    void load();
    void unload();

    void activate();
    void deactivate();

    bool is_loaded() const { return is_loaded_; }
    bool is_active() const { return is_active_; }

    const std::string name() const {
        return name_;
    }

    void set_name(const std::string& name) {
        name_ = name;
    }

    /* Whether or not the scene should be unloaded when it's deactivated
     * this is the default behaviour */
    bool unload_on_deactivate() const { return unload_on_deactivate_; }
    void set_unload_on_deactivate(bool v) {
        unload_on_deactivate_ = v;
    }

    template<typename T>
    T* start_service() {
        size_t info = typeid(T).hash_code();
        if(services_.count(info)) {
            return static_cast<T*>(services_.at(info).get());
        }

        auto service = std::make_shared<T>();

        // FIXME: start signal

        services_.insert(std::make_pair(info, service));
        return service.get();
    }

    template<typename T>
    bool stop_service() {
        size_t info = typeid(T).hash_code();
        auto it = services_.find(info);
        if(it != services_.end()) {
            return false;
        }

        // FIXME: Stop signal
        services_.erase(it);
        return true;
    }

    template<typename T>
    T* find_service() const {
        auto info = typeid(T).hash_code();
        auto it = services_.find(info);
        if(it != services_.end()) {
            return static_cast<T*>(it->second.get());
        }

        return nullptr;
    }

    const std::set<StageNode*> stray_nodes() const {
        return stray_nodes_;
    }

protected:
    virtual void on_load() = 0;
    virtual void on_unload() {}
    virtual void on_activate() {}
    virtual void on_deactivate() {}

private:
    void on_fixed_update(float step) override;

    void register_builtin_nodes();
    std::unordered_map<size_t, std::shared_ptr<Service>> services_;

    virtual void on_pre_load() {}
    virtual void on_post_unload() {}

    bool is_loaded_ = false;
    bool is_active_ = false;
    bool unload_on_deactivate_ = true;    

    std::string name_;

    Window* window_;
    InputManager* input_;
    Application* app_;
    SceneManager* scene_manager_ = nullptr;
    SceneCompositor compositor_;

    AssetManager assets_;

    friend class SceneManager;

    std::vector<any> load_args;

    void on_clean_up() override {
        unload();
    }

    // So that stage nodes can call queue_clean_up
    friend class StageNode;

    void clean_up_destroyed_objects();
    void queue_clean_up(StageNode* node);
    std::list<StageNode*> queued_for_clean_up_;
    std::set<StageNode*> stray_nodes_;

    LightingSettings lighting_;

    /* Don't allow overriding on_create in subclasses, currently
     * the hook for that is init + load */
    bool on_create(ConstructionArgs*) override final {
        return true;
    }

    void do_generate_renderables(
        batcher::RenderQueue*,
        const Camera*, const Viewport*, const DetailLevel) override final {
        /* Do nothing, Scenes don't create renderables.. for now */
    }

    /* Scenes don't care about AABBs */
    const AABB& aabb() const override final {
        static AABB ret;
        return ret;
    }



protected:
    /* Returns the number of arguments passed when loading */
    std::size_t load_arg_count() const;

    template<typename T>
    T get_load_arg(int i) {
        return any_cast<T>(load_args[i]);
    }

public:
    S_DEFINE_PROPERTY(window, &Scene::window_);
    S_DEFINE_PROPERTY(app, &Scene::app_);
    S_DEFINE_PROPERTY(input, &Scene::input_);
    S_DEFINE_PROPERTY(scenes, &Scene::scene_manager_);
    S_DEFINE_PROPERTY(compositor, &Scene::compositor_);
    S_DEFINE_PROPERTY(lighting, &Scene::lighting_);
    S_DEFINE_PROPERTY(assets, &Scene::assets_);
};



}

#endif // SCENE_H
