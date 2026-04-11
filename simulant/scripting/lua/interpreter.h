#pragma once

// clang-format off
#include "../interpreter.h"
#include "minilua.h"
#include "LuaBridge.h"
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

    LuaStageNode(luabridge::LuaRef instance) :
        StageNode(instance["scene"], instance["node_type"]),
        ref_(new Pimpl(instance)) {}

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

    // Getters exposed to LuaBridge as properties so that Lua subclasses can
    // read obj.scene, obj.node_type, and obj.transform, and so that the
    // private LuaStageNode(LuaRef) constructor can read them back from the
    // instance.
    Scene* lua_get_scene() const { return get_scene(); }
    StageNodeType lua_get_node_type() const { return node_type(); }
    Transform* lua_get_transform() const { return get_transform(); }

    virtual bool on_create(Params params) override {
        if(!ref_ || !ref_->instance) {
            return true;
        }

        luabridge::LuaRef method = ref_->instance["on_create"];
        if(method.isNil() || !method.isFunction()) {
            return true;
        }
        // Params is not registered in LuaBridge, so we do not forward it to
        // Lua.  Scripts that need construction arguments should read them from
        // self._cpp (scene, transform, etc.) or from a companion table passed
        // via the node's Meta instead.
        auto call = method.callable<bool(luabridge::LuaRef)>();
        return *call(ref_->instance);
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
