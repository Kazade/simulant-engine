#include "simulant/c/stage_node_ext.h"

#include "simulant/scenes/scene.h"
#include "simulant/utils/hash/fnv1.h"

namespace {

/* The one concrete C++ type behind every C-defined stage node "subclass".
 * Which vtable and name a given instance uses is decided per-type at
 * registration time (captured by the constructor closure handed to
 * StageNodeManager::register_stage_node()), not by this class itself --
 * the same pattern Simulant's own LuaStageNode uses for script-defined
 * node types.
 */
class CStageNode: public smlt::StageNode {
public:
    CStageNode(smlt::Scene* owner, smlt::StageNodeType node_type, std::string type_name,
              smlt_stage_node_vtable_t vtable) :
        StageNode(owner, node_type),
        type_name_(std::move(type_name)),
        vtable_(vtable) {}

    ~CStageNode() override {
        if(vtable_.on_deleted) {
            vtable_.on_deleted(user_data_);
        }
    }

    const char* node_type_name() const override {
        return type_name_.c_str();
    }

    std::set<smlt::NodeParam> node_params() const override {
        return smlt::get_node_params<CStageNode>();
    }

    void set_user_data(void* user_data) {
        user_data_ = user_data;
    }

    void* user_data() const {
        return user_data_;
    }

protected:
    bool on_destroy() override {
        if(vtable_.on_destroy) {
            return vtable_.on_destroy(self(), user_data_);
        }
        return true;
    }

    void on_update(float dt) override {
        if(vtable_.on_update) {
            vtable_.on_update(self(), dt, user_data_);
        }
    }

    void on_fixed_update(float step) override {
        if(vtable_.on_fixed_update) {
            vtable_.on_fixed_update(self(), step, user_data_);
        }
    }

    void on_late_update(float dt) override {
        if(vtable_.on_late_update) {
            vtable_.on_late_update(self(), dt, user_data_);
        }
    }

    void on_parent_set(const smlt::StageNode* oldp, const smlt::StageNode* newp) override {
        if(vtable_.on_parent_set) {
            vtable_.on_parent_set(
                self(), reinterpret_cast<const smlt_stage_node_t*>(oldp),
                reinterpret_cast<const smlt_stage_node_t*>(newp), user_data_);
        }
    }

private:
    smlt_stage_node_t* self() {
        return reinterpret_cast<smlt_stage_node_t*>(this);
    }

    std::string type_name_;
    smlt_stage_node_vtable_t vtable_;
    void* user_data_ = nullptr;
};

} // namespace

extern "C" {

uint32_t smlt_stage_node_register_type(smlt_scene_t* scene, const char* type_name,
                                       const smlt_stage_node_vtable_t* vtable) {
    auto* real_scene = reinterpret_cast<smlt::Scene*>(scene);
    auto type_id = smlt::fnv1<uint32_t>::hash(type_name);
    std::string name_copy(type_name);
    // Copied by value: registration only stores a closure that runs later
    // (the next time this type is instantiated), by which point a vtable
    // built on the caller's stack (e.g. inside an init() callback) may
    // already be gone.
    smlt_stage_node_vtable_t vtable_copy = vtable ? *vtable : smlt_stage_node_vtable_t{};

    real_scene->register_stage_node(
        type_id, type_name, sizeof(CStageNode), alignof(CStageNode),
        [real_scene, type_id, name_copy, vtable_copy](void* mem) -> smlt::StageNode* {
            return new(mem) CStageNode(real_scene, type_id, name_copy, vtable_copy);
        },
        [](smlt::StageNode* node) { static_cast<CStageNode*>(node)->~CStageNode(); });

    return type_id;
}

smlt_stage_node_t* smlt_stage_node_create_custom(smlt_scene_t* scene, smlt_stage_node_t* parent,
                                                 uint32_t type_id) {
    auto* real_scene = reinterpret_cast<smlt::Scene*>(scene);
    smlt::StageNode* node = real_scene->create_node(type_id, smlt::Params(), nullptr);
    if(node && parent) {
        node->set_parent(reinterpret_cast<smlt::StageNode*>(parent));
    }
    return reinterpret_cast<smlt_stage_node_t*>(node);
}

void smlt_stage_node_set_user_data(smlt_stage_node_t* self, void* user_data) {
    auto* node = dynamic_cast<CStageNode*>(reinterpret_cast<smlt::StageNode*>(self));
    if(node) {
        node->set_user_data(user_data);
    }
}

void* smlt_stage_node_get_user_data(const smlt_stage_node_t* self) {
    auto* node =
        dynamic_cast<const CStageNode*>(reinterpret_cast<const smlt::StageNode*>(self));
    return node ? node->user_data() : nullptr;
}

} // extern "C"
