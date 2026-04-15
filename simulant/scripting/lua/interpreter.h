#pragma once

// clang-format off
#include "../interpreter.h"

/* Suppress warnings from third-party headers (minilua, LuaBridge) */
#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
#pragma GCC diagnostic ignored "-Wextra"
#pragma GCC diagnostic ignored "-Wunused-parameter"
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
#include "../../generic/optional.h"
#include "../../nodes/stage_node.h"
#include "../../path.h"
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

    virtual std::set<NodeParam> node_params() const override {
        return params_;
    }

private:
    std::string node_type_name_;
    std::set<NodeParam> params_;
};

class LuaInterpreter: public RefCounted<LuaInterpreter>, public Interpreter {
public:
    bool load_string(const char* data);
    bool load_file(const Path& script);

    smlt::optional<luabridge::LuaRef> get_global(const char* name);

    lua_State* lua_state() const { return state_; }

private:
    bool on_init() override;

    void on_update(float dt) override {}

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
