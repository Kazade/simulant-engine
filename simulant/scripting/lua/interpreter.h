#pragma once

// clang-format off
#include "../interpreter.h"

/* Suppress warnings from third-party headers (minilua, LuaBridge) */
#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
#pragma GCC diagnostic ignored "-Wextra"
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wunused-but-set-parameter"
#pragma GCC diagnostic ignored "-Wunused-but-set-variable"
#pragma GCC diagnostic ignored "-Wsign-compare"
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wdouble-promotion"
#if defined(__clang__)
#pragma GCC diagnostic ignored "-Wc++98-compat-pedantic"
#pragma GCC diagnostic ignored "-Wreserved-identifier"
#pragma GCC diagnostic ignored "-Wunused-macros"
#endif
#endif

#define LUA_USE_JUMPTABLE 0
#if defined(__DREAMCAST__) || defined(__PSP__) || defined(__XBOX__)
#define LUA_32BITS 1
#endif
#include "minilua.h"
#include "LuaBridge.h"

#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic pop
#endif
// clang-format on
#include "../../application.h"
#include "../../generic/optional.h"
#include "../../nodes/prefab_instance.h"
#include "../../nodes/stage_node.h"
#include "../../path.h"
#include "../../scenes/scene.h"
#include "bindings.h"

namespace smlt {

class LuaStageNode: public StageNode {
private:
    struct Pimpl {
        Pimpl(const luabridge::LuaRef& instance) :
            instance(instance) {}

        luabridge::LuaRef instance;
    };

    LuaStageNode(Scene* scene, StageNodeType node_type,
                 std::string node_type_name, std::set<NodeParam> params,
                 luabridge::LuaRef instance) :
        StageNode(scene, node_type),
        ref_(new Pimpl(instance)),
        node_type_name_(node_type_name),
        params_(params) {}

    Pimpl* ref_ = nullptr;

    friend class StageNodeManager;

public:
    LuaStageNode(Scene* scene, StageNodeType node_type,
                 std::string node_type_name, std::set<NodeParam> params) :
        StageNode(scene, node_type),
        node_type_name_(node_type_name),
        params_(params) {

            // FIXME: Ideally this would be called in the Lua constructor
            init();
        }

    // Getters exposed to LuaBridge as properties so that Lua code can
    // read obj.scene, obj.node_type, obj.transform and obj.assets via the
    // _cpp_node forwarding in define_node's __index metamethod.
    Scene* lua_get_scene() const { return get_scene(); }
    StageNodeType lua_get_node_type() const { return node_type(); }
    Transform* lua_get_transform() const { return get_transform(); }
    AssetManager* lua_get_assets() const;

    ~LuaStageNode() override {
        delete ref_;
    }

    virtual bool on_create(Params params) override {
        // Let the base class handle common position/orientation/scale params.
        if(!StageNode::on_create(params)) {
            return false;
        }

        if(!ref_ || !ref_->instance) {
            return true;
        }

        luabridge::LuaRef method = ref_->instance["on_create"];
        if(method.isNil() || !method.isFunction()) {
            return true;
        }

        // Build a Lua table from the node-specific declared params.
        // We iterate over the declared params (node_params()), apply
        // defaults for missing optional params, fail if a required param is
        // absent, and convert the remaining values to Lua-compatible types.
        lua_State* L = ref_->instance.state();
        luabridge::LuaRef params_table = luabridge::newTable(L);

        for(const auto& param: node_params()) {
            const auto& pname = param.name();
            auto raw_val = params.raw(pname);

            // Apply declared default if the caller didn't provide this param.
            if(!raw_val && param.default_value()) {
                raw_val = param.default_value();
            }

            if(!raw_val) {
                if(param.is_required()) {
                    S_ERROR("Required param '{0}' not provided for Lua node '{1}'",
                            pname, node_type_name_);
                    return false;
                }
                continue;  // Optional, no default — skip
            }

            // Convert the C++ ParamValue to a Lua-compatible type.
            std::visit([&](auto&& v) {
                using T = std::decay_t<decltype(v)>;
                if constexpr(std::is_same_v<T, float>) {
                    params_table[pname.c_str()] = v;
                } else if constexpr(std::is_same_v<T, int>) {
                    params_table[pname.c_str()] = v;
                } else if constexpr(std::is_same_v<T, bool>) {
                    params_table[pname.c_str()] = (bool)v;
                } else if constexpr(std::is_same_v<T, std::string>) {
                    params_table[pname.c_str()] = v;
                } else if constexpr(std::is_same_v<T, FloatArray>) {
                    luabridge::LuaRef arr = luabridge::newTable(L);
                    for(int i = 0; i < (int)v.size(); ++i) {
                        arr[i + 1] = v[i];
                    }
                    params_table[pname.c_str()] = arr;
                }
                // Complex types (MeshPtr, TexturePtr, etc.) are not converted.
            }, raw_val.value());
        }

        auto call = method.callable<bool(luabridge::LuaRef, luabridge::LuaRef)>();
        return call(ref_->instance, params_table).valueOr(false);
    }

    void on_update(float dt) override {
        if(!ref_ || !ref_->instance) {
            return;
        }

        luabridge::LuaRef method = ref_->instance["on_update"];
        if(method.isNil() || !method.isFunction()) {
            return;
        }
        method(ref_->instance, dt);
    }

    void on_fixed_update(float step) override {
        if(!ref_ || !ref_->instance) {
            return;
        }

        luabridge::LuaRef method = ref_->instance["on_fixed_update"];
        if(method.isNil() || !method.isFunction()) {
            return;
        }
        method(ref_->instance, step);
    }

    void on_late_update(float dt) override {
        if(!ref_ || !ref_->instance) {
            StageNode::on_late_update(dt);
            return;
        }

        luabridge::LuaRef method = ref_->instance["on_late_update"];
        if(method.isNil() || !method.isFunction()) {
            StageNode::on_late_update(dt);
            return;
        }
        method(ref_->instance, dt);
    }

    bool on_destroy() override {
        if(!ref_ || !ref_->instance) {
            return StageNode::on_destroy();
        }

        luabridge::LuaRef method = ref_->instance["on_destroy"];
        if(method.isNil() || !method.isFunction()) {
            return StageNode::on_destroy();
        }

        auto call = method.callable<bool(luabridge::LuaRef)>();
        return call(ref_->instance).valueOr(true);
    }

    void _clean_up() override {
        if(ref_ && ref_->instance) {
            luabridge::LuaRef method = ref_->instance["on_clean_up"];
            if(!method.isNil() && method.isFunction()) {
                method(ref_->instance);
            }
        }
        StageNode::_clean_up();
    }

    void on_parent_set(const StageNode* oldp, const StageNode* newp) override {
        if(!ref_ || !ref_->instance) {
            StageNode::on_parent_set(oldp, newp);
            return;
        }

        luabridge::LuaRef method = ref_->instance["on_parent_set"];
        if(method.isNil() || !method.isFunction()) {
            StageNode::on_parent_set(oldp, newp);
            return;
        }
        method(ref_->instance);
    }

    void on_transformation_changed() override {
        if(!ref_ || !ref_->instance) {
            StageNode::on_transformation_changed();
            return;
        }

        luabridge::LuaRef method = ref_->instance["on_transformation_changed"];
        if(method.isNil() || !method.isFunction()) {
            StageNode::on_transformation_changed();
            return;
        }
        method(ref_->instance);
    }

    virtual const char* node_type_name() const override {
        return node_type_name_.c_str();
    }

    // Lua-scripted node types have no way (yet) to declare an intended
    // usage of their own, so default to the permissive option.
    virtual StageNodeUsage node_usage() const override {
        return STAGE_NODE_USAGE_EITHER;
    }

    virtual std::set<NodeParam> node_params() const override {
        return params_;
    }

private:
    std::string node_type_name_;
    std::set<NodeParam> params_;
};

/* A Lua stage-node script to auto-register (via register_stage_node) when a
 * LuaScene is constructed, before any content is created. This lets a scene
 * bring its own Lua-authored node types along with it (used by scenes
 * registered from a gltf's SMLT_scene_script extension, where a
 * "stage_node"-type entry needs to exist by the time the gltf's own nodes
 * are instantiated). */
struct LuaStageNodeScriptDef {
    std::string source;
    std::string class_name;
};

class LuaScene: public Scene {
private:
    struct Pimpl {
        Pimpl(const luabridge::LuaRef& instance) :
            instance(instance) {}

        luabridge::LuaRef instance;
    };

    Pimpl* ref_ = nullptr;
    PrefabPtr prefab_;

    friend class SceneManager;

public:
    LuaScene(Window* window, luabridge::LuaRef instance,
             PrefabPtr prefab = PrefabPtr(),
             const std::vector<LuaStageNodeScriptDef>& stage_node_scripts = {}) :
        Scene(window),
        ref_(new Pimpl(instance)),
        prefab_(prefab) {

        for(auto& def: stage_node_scripts) {
            if(!register_stage_node(def.source.c_str(), def.class_name.c_str())) {
                S_ERROR("Failed to register stage node script class '{0}'",
                        def.class_name);
            }
        }
    }

    ~LuaScene() override {
        delete ref_;
    }

    void on_load() override {
        // Instantiate the gltf's own node graph (if any) before running the
        // script's own on_load, exactly as if it were a PrefabInstance
        // created first thing in a hand-written C++ Scene::on_load().
        if(prefab_) {
            create_child<PrefabInstance>(prefab_);
        }

        // The scene script's own on_load() is game logic (it may create
        // gameplay-only cameras/layers/nodes) - editors want the authored
        // node graph above without running it.
        if(!smlt::get_app()->is_editor_mode()) {
            call_lua_method("on_load");
        }
    }

    void on_unload() override {
        if(!call_lua_method("on_unload")) {
            Scene::on_unload();
        }
    }

    void on_activate() override {
        if(!call_lua_method("on_activate")) {
            Scene::on_activate();
        }
    }

    void on_deactivate() override {
        if(!call_lua_method("on_deactivate")) {
            Scene::on_deactivate();
        }
    }

private:
    // Returns true if a Lua method of this name was found and invoked
    // (regardless of whether the call itself raised a Lua error, which is
    // logged rather than silently discarded).
    bool call_lua_method(const char* name) {
        if(!ref_ || !ref_->instance) {
            return false;
        }

        luabridge::LuaRef method = ref_->instance[name];
        if(method.isNil() || !method.isFunction()) {
            return false;
        }

        std::string err;
        auto handler = [&err](lua_State* L) -> int {
            if(lua_gettop(L) > 0 && lua_isstring(L, -1)) {
                err = lua_tostring(L, -1);
            }
            return 1;
        };

        if(!method.callWithHandler(handler, ref_->instance)) {
            S_ERROR("Lua error in {0}: {1}", name, err);
        }

        return true;
    }
};

class LuaInterpreter: public RefCounted<LuaInterpreter>, public Interpreter {
public:
    bool load_string(const char* data);
    bool load_file(const Path& script);

    smlt::optional<luabridge::LuaRef> get_global(const char* name);

    lua_State* lua_state() const { return state_; }

private:
    bool on_init() override;

    void on_update(float /*dt*/) override {}

    void on_clean_up() override {
        lua_close(state_);
    }

    lua_State* state_ = nullptr;

    static void* l_alloc(void* ud, void* ptr, size_t osize, size_t nsize) {
        (void)ud;
        (void)osize; /* not used */
        if(nsize == 0) {
            free(ptr);
            return NULL;
        } else {
            return realloc(ptr, nsize);
        }
    }

    friend luabridge::LuaRef stage_node_meta(std::string name);
};

} // namespace smlt
