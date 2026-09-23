#pragma once

extern "C" {
    struct lua_State;
}

#include "../utils/params.h"
#include "helpers.h"
#include "stage_node.h"
#include "stage_node_storage.h"
#include <functional>

namespace smlt {

class Params;

typedef std::function<StageNode*(void*)> StageNodeConstructFunction;
typedef std::function<void(StageNode*)> StageNodeDestructFunction;
typedef std::function<std::set<NodeParam>()> StageNodeParamQueryFunction;

class StageNodeManager;

/* Registers a factory for a native (C++) stage-node type under a plain
 * string name, so that scripts - notably Lua scenes/nodes, which cannot
 * instantiate C++ templates - can register that type on a scene by name
 * (see Scene::register_native_stage_node). Applications call this once at
 * startup, e.g.:
 *
 *   register_native_stage_node_type("my_node", [](StageNodeManager* m) {
 *       return m->register_stage_node<MyNode>();
 *   });
 */
typedef std::function<bool(StageNodeManager*)> NativeStageNodeRegistrar;

void register_native_stage_node_type(const std::string& name,
                                     NativeStageNodeRegistrar registrar);

/* Invokes the factory registered for `name` against `manager`. Returns
 * false (logging an error) if no such type has been registered. */
bool register_native_stage_node_type_on(StageNodeManager* manager,
                                        const std::string& name);

struct StageNodeTypeInfo {
    StageNodeType type;
    std::string name;
    std::size_t size_in_bytes;
    std::size_t alignment;

    StageNodeConstructFunction constructor;
    StageNodeDestructFunction destructor;

    // Returns this type's on_create() param schema (name/type/default/
    // required) without needing an instance - lets tooling (e.g. the
    // editor's "create node" dialog) build a form for any registered type
    // purely from its name. Bound in register_stage_node<T>() to
    // get_node_params<T>(), which is otherwise only reachable when T is
    // known at compile time.
    StageNodeParamQueryFunction param_query;

    // Mirrors T::Meta::usage_kind (see S_DEFINE_STAGE_NODE_META) - whether
    // this type is meant to be created standalone, only ever attached as a
    // mixin, or either. Defaults to the permissive EITHER for types that
    // don't set it explicitly (e.g. Lua-scripted node types, which have no
    // way to declare this yet).
    StageNodeUsage usage = STAGE_NODE_USAGE_EITHER;
};

class Scene;

template<typename T>
class HasRegisterFunc {
private:
    typedef char yes[1];
    typedef char no[2];

    template<typename C>
    static yes& test(decltype(&C::on_register)*);

    template<typename>
    static no& test(...);

public:
    static const bool value = sizeof(test<T>(0)) == sizeof(yes);
};

class StageNodeManager {
private:
    template<typename T>
    static StageNode* standard_new(Scene* scene, void* mem) {
        return new(mem) T(scene);
    }

    template<typename T>
    static void standard_delete(StageNode* mem) {
        T* to_delete = dynamic_cast<T*>(mem);
        to_delete->~T();
    }

    struct NodeData {
        void* alloc_base;
        StageNode* ptr;

        NodeData(void* alloc_base, StageNode* ptr) :
            alloc_base(alloc_base), ptr(ptr) {}
    };

    std::unordered_map<StageNodeType, StageNodeTypeInfo> registered_nodes_;
    std::unordered_map<StageNodeID, NodeData> all_nodes_;

    StageNodeStorage node_storage_;
    std::unordered_map<StageNodeType, std::vector<StageNode*>> nodes_by_type_;

protected:
    bool clean_up_node(StageNode* node);

    Scene* scene_;

    virtual void on_stage_node_inserted(StageNode* node) {
        nodes_by_type_[node->node_type()].push_back(node);
    }

    virtual void on_stage_node_erased(StageNode* node) {
        auto& arr = nodes_by_type_[node->node_type()];
        arr.erase(std::remove(arr.begin(), arr.end(), node), arr.end());
    }

public:
    StageNodeManager(Scene* scene) :
        scene_(scene) {}

    virtual ~StageNodeManager();

    const std::vector<StageNode*>& nodes_by_type(StageNodeType type) {
        return nodes_by_type_[type];
    }

    /* Constant-time node lookup by ID */
    StageNode* get_node(StageNodeID id) const {
        return all_nodes_.at(id).ptr;
    }

    /* Constant-time node existence check */
    bool has_node(StageNodeID id) const {
        return all_nodes_.count(id) > 0;
    }

    /* Non-template API does the work for easier binding with other languages */
    StageNode* create_node(StageNodeType type, const Params& params,
                           StageNode* base);
    StageNode* create_node(const std::string& name, const Params& params,
                           StageNode* base);

    template<typename T, typename... Args>
    T* _create_node(const WithBase& base, Args&&... args) {
        Params params;

        auto node_params = get_node_params<T>();
        params_unpack(params, node_params.begin(), node_params.end(),
                      std::forward<Args>(args)...);

        return (T*)create_node(T::Meta::node_type, params, base.base);
    }

    template<typename T, typename... Args>
    T* _create_node(const WithBase& base, Params args) {
        return (T*)create_node(T::Meta::node_type, args, base.base);
    }

    template<typename T, typename... Args>
    T* create_node(Args&&... args) {
        return _create_node<T>(WithBase(nullptr), std::forward<Args>(args)...);
    }

    template<typename T>
    T* create_node(Params args) {
        return (T*)_create_node<T>(WithBase(nullptr), args);
    }

    template<typename T>
    T* create_node() {
        Params args;
        return (T*)_create_node<T>(WithBase(nullptr), args);
    }

    bool register_stage_node(const Path& script_file, const char* class_name);
    bool register_stage_node(const char* script_data, const char* class_name);

private:
    /* Shared implementation used by both register_stage_node overloads after
       the Lua state has been loaded. */
    bool register_stage_node_from_lua_state(lua_State* L, const char* class_name);

public:
    bool register_stage_node(StageNodeType type, const char* name,
                             std::size_t size_in_bytes, std::size_t alignment,
                             StageNodeConstructFunction construct_func,
                             StageNodeDestructFunction destruct_func) {

        if(registered_nodes_.find(type) != registered_nodes_.end()) {
            S_WARN("Attempted to register duplicate node: {0}", type);
            return false;
        }

        StageNodeTypeInfo info = {type,      name,           size_in_bytes,
                                  alignment, construct_func, destruct_func};

        registered_nodes_.insert(std::make_pair(type, info));
        S_DEBUG("Registered new stage node type: {0}", type);
        return true;
    }

    // All node types registered on this scene (built-ins from
    // Scene::register_builtin_nodes(), plus any Lua-scripted types the
    // scene registered itself) - e.g. for the editor's "create node" UI
    // to enumerate what's available and query each type's param schema.
    const std::unordered_map<StageNodeType, StageNodeTypeInfo>&
        registered_nodes() const {
        return registered_nodes_;
    }

    optional<StageNodeTypeInfo>
        registered_stage_node_info(const std::string& name) {
        for(auto& p: registered_nodes_) {
            if(p.second.name == name) {
                return p.second;
            }
        }

        return no_value;
    }

    template<typename T>
    bool register_stage_node() {
        // If the stage node has an on_register callback, then call
        // it with this scene
        if constexpr(HasRegisterFunc<T>::value) {
            T::on_register(scene_);
        }

        bool ok = register_stage_node(
            T::Meta::node_type, T::Meta::name, sizeof(T), alignof(T),
            std::bind(&StageNodeManager::standard_new<T>, scene_,
                     std::placeholders::_1),
            &StageNodeManager::standard_delete<T>);

        if(ok) {
            // Copy into a local first: StageNodeManager::at() takes its key
            // by const&, and binding that reference directly to
            // T::Meta::node_type (an in-class-initialized static const,
            // never given an out-of-line definition) would odr-use it and
            // fail to link.
            StageNodeType type = T::Meta::node_type;
            registered_nodes_.at(type).param_query =
                []() { return get_node_params<T>(); };
            registered_nodes_.at(type).usage = T::Meta::usage_kind;
        }

        return ok;
    }
};

} // namespace smlt
