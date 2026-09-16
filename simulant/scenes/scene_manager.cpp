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
#include "../asset_manager.h"
#include "../loaders/gltf_loader.h"
#include "../window.h"
#include "../application.h"
#include "../scripting/lua/interpreter.h"

namespace smlt {

SceneManager::SceneManager(Window *window):
    window_(window) {

}

SceneManager::~SceneManager() {

}

void SceneManager::destroy_all() {
    /* Unload all the routes */
    for(auto& route: routes_) {
        route.second->unload();
        scenes_queued_for_destruction_.insert(route.second);
    }

    current_scene_.reset();
}

void SceneManager::clean_destroyed_scenes() {
    /* Destroy any scenes that have been queued */
    scenes_queued_for_destruction_.clear();
}

void SceneManager::late_update(float dt) {
    auto app = get_app();
    auto window = (app) ? app->window.get() : nullptr;
    if(!(window && window->has_focus())) {
        // if paused, send deltatime as 0.0.
        // it's still accessible through window->time_keeper if the user needs it
        dt = 0.0;
    }

    if(active_scene()) {
        active_scene()->late_update(dt);

        /* Anything destroyed must now be *really* destroyed */
        active_scene()->clean_up_destroyed_objects();
    }

    clean_destroyed_scenes();

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
        scene->unload();
        scene->load_args.clear();
        scene->destroy();

        // Erase from routes so any further get_or_create triggers
        // the factory function
        routes_.erase(it);

        /* Destroy the scene once it's been unloaded but do
         * it after late_update so that any queued destructions
         * from unload can happen before we destroy the scene
         */
        scenes_queued_for_destruction_.insert(scene);
        current_scene_ = nullptr;
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
        p.second->unload();
    }
    routes_.clear();
    scene_factories_.clear();
}

bool SceneManager::register_scene(const Path& script, const char* class_name) {
    auto lua = smlt::get_app()->ensure_lua_ready();

    if(!lua->load_file(script)) {
        return false;
    }

    return register_scene_from_lua_state(lua->lua_state(), class_name);
}

bool SceneManager::register_scene(const char* script_data,
                                  const char* class_name) {
    auto lua = smlt::get_app()->ensure_lua_ready();

    if(!lua->load_string(script_data)) {
        return false;
    }

    return register_scene_from_lua_state(lua->lua_state(), class_name);
}

bool SceneManager::register_scene_from_lua_state(
    lua_State* L, const char* class_name, Path gltf_path,
    const std::vector<std::pair<std::string, std::string>>&
        stage_node_scripts) {
    luabridge::LuaRef klass = luabridge::getGlobal(L, class_name);
    if(klass.isNil()) {
        S_ERROR("Unable to find specified class in Lua file: {0}", class_name);
        return false;
    }

    luabridge::LuaRef meta = klass["Meta"];
    if(meta.isNil()) {
        S_ERROR("Lua class '{0}' is missing Meta table", class_name);
        return false;
    }

    std::string name = meta["name"].cast<std::string>().valueOr("");
    if(name.empty()) {
        S_ERROR("Lua class '{0}' Meta table is missing name", class_name);
        return false;
    }

    // Capture the raw lua_State* and the class name string rather than a
    // luabridge::LuaRef, for the same reason as
    // StageNodeManager::register_stage_node_from_lua_state: a LuaRef
    // destroyed after lua_close() (which happens when the interpreter is
    // torn down) would call luaL_unref on already-freed Lua memory.
    std::string klass_name(class_name);

    // Converted once here (rather than captured as pairs and converted on
    // every instantiation) since LuaStageNodeScriptDef is only visible in
    // this .cpp — the header keeps this method's signature Lua-agnostic.
    std::vector<LuaStageNodeScriptDef> node_script_defs;
    for(auto& p: stage_node_scripts) {
        node_script_defs.push_back({p.first, p.second});
    }

    _store_scene_factory(
        name, [L, klass_name, name, gltf_path, node_script_defs,
               this](Window* window) -> ScenePtr {
        lua_getglobal(L, klass_name.c_str());
        if(lua_isnil(L, -1)) {
            lua_pop(L, 1);
            S_ERROR("Lua class not found: {0}", klass_name);
            return ScenePtr();
        }
        luabridge::LuaRef klass = luabridge::LuaRef::fromStack(L, -1);
        lua_pop(L, 1);

        auto constructor = klass["new"];
        try {
            // cls:new(window) returns a plain Lua wrapper table; _cpp_node
            // is not set yet — that happens below once the real LuaScene
            // has been constructed.
            luabridge::LuaRef instance =
                *constructor.call<luabridge::LuaRef>(klass, window);

            std::shared_ptr<LuaScene> ret(
                new LuaScene(window, instance, PrefabPtr(), node_script_defs),
                &SceneManager::deleter<LuaScene>);

            // Wire the wrapper table to the real C++ scene using a raw
            // table set so that __newindex metamethods (if any) are
            // bypassed. Pushed as a plain Scene* so it uses the smlt.Scene
            // binding (create_child, assets, window, etc).
            instance.rawsetField("_cpp_node",
                                 static_cast<Scene*>(ret.get()));

            // Load the gltf prefab lazily - at scene instantiation, not at
            // registration, so unused routes don't hold heavyweight asset
            // data in memory - and into this scene's own asset manager
            // (rather than the shared one) so the assets' lifetime matches
            // the scene's, the same as a hand-written Scene::on_load()
            // calling assets->load_prefab(...).
            if(!gltf_path.str().empty()) {
                ret->prefab_ = ret->assets->load_prefab(gltf_path);
                if(!ret->prefab_) {
                    S_ERROR("Unable to load gltf scene graph: {0}",
                            gltf_path.str());
                    return ScenePtr();
                }
            }

            if(!ret->init()) {
                S_ERROR("Failed to initialize the Scene");
                return ScenePtr();
            }

            ret->set_name(name);
            ret->scene_manager_ = this;
            return ret;
        } catch(const std::exception& e) {
            S_ERROR("Lua error: {0}", e.what());
            return ScenePtr();
        }
    });

    return true;
}

bool SceneManager::register_scene(const Path& gltf_path) {
    auto loader = smlt::get_app()->loader_for(gltf_path);
    if(!loader) {
        S_ERROR("Unable to find a loader for: {0}", gltf_path.str());
        return false;
    }

    auto gltf_loader = std::dynamic_pointer_cast<loaders::GLTFLoader>(loader);
    if(!gltf_loader) {
        S_ERROR("Not a gltf/glb file: {0}", gltf_path.str());
        return false;
    }

    std::vector<loaders::GLTFLoader::SceneScriptDef> scripts;
    if(!gltf_loader->find_scene_scripts(scripts)) {
        return false;
    }

    const loaders::GLTFLoader::SceneScriptDef* scene_script = nullptr;
    std::vector<std::pair<std::string, std::string>> stage_node_scripts;
    for(auto& s: scripts) {
        if(s.type == "scene") {
            if(scene_script) {
                S_ERROR("gltf file '{0}' declares more than one "
                        "SMLT_scene_script of type 'scene'",
                        gltf_path.str());
                return false;
            }
            scene_script = &s;
        } else {
            stage_node_scripts.push_back({s.source, s.class_name});
        }
    }

    if(!scene_script) {
        S_ERROR("gltf file '{0}' has no SMLT_scene_script of type 'scene'",
                gltf_path.str());
        return false;
    }

    // The prefab is loaded lazily inside register_scene_from_lua_state
    // (at scene instantiation, not registration) so unused routes don't
    // hold heavyweight asset data in memory.

    auto lua = smlt::get_app()->ensure_lua_ready();
    if(!lua->load_string(scene_script->source.c_str())) {
        return false;
    }

    return register_scene_from_lua_state(lua->lua_state(),
                                         scene_script->class_name.c_str(),
                                         gltf_path, stage_node_scripts);
}

}
