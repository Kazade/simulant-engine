#include "simulant/c/scene_ext.h"

#include "simulant/asset_manager.h"
#include "simulant/scenes/scene_manager.h"

#include <string>

namespace {

class CScene: public smlt::Scene {
public:
    CScene(smlt::Window* window, smlt_scene_vtable_t vtable, void* ctx) :
        Scene(window),
        vtable_(vtable) {
        user_data_ = vtable_.create_user_data ? vtable_.create_user_data(self(), ctx) : ctx;
    }

    ~CScene() override {
        if(vtable_.delete_user_data) {
            vtable_.delete_user_data(user_data_);
        }
    }

    void set_user_data(void* user_data) {
        user_data_ = user_data;
    }

    void* user_data() const {
        return user_data_;
    }

protected:
    void on_load() override {
        if(vtable_.on_load) {
            vtable_.on_load(self(), user_data_);
        }
    }

    void on_unload() override {
        if(vtable_.on_unload) {
            vtable_.on_unload(self(), user_data_);
        }
    }

    void on_activate() override {
        if(vtable_.on_activate) {
            vtable_.on_activate(self(), user_data_);
        }
    }

    void on_deactivate() override {
        if(vtable_.on_deactivate) {
            vtable_.on_deactivate(self(), user_data_);
        }
    }

    void on_update(float dt) override {
        // Scene::on_update() drives service updates and the StageNode
        // chain -- it must always run, not just when there's no callback.
        Scene::on_update(dt);
        if(vtable_.on_update) {
            vtable_.on_update(self(), dt, user_data_);
        }
    }

    void on_fixed_update(float step) override {
        Scene::on_fixed_update(step);
        if(vtable_.on_fixed_update) {
            vtable_.on_fixed_update(self(), step, user_data_);
        }
    }

private:
    smlt_scene_t* self() {
        return reinterpret_cast<smlt_scene_t*>(this);
    }

    smlt_scene_vtable_t vtable_;
    void* user_data_;
};

} // namespace

extern "C" {

void smlt_scene_register_type(smlt_scene_manager_t* scene_manager, const char* name,
                              const smlt_scene_vtable_t* vtable, void* ctx) {
    auto* manager = reinterpret_cast<smlt::SceneManager*>(scene_manager);
    // Copied by value: register_scene()'s factory closure isn't invoked
    // until this route is first activated, by which point a vtable built
    // on the caller's stack (e.g. inside an init() callback) may already
    // be gone.
    smlt_scene_vtable_t vtable_copy = vtable ? *vtable : smlt_scene_vtable_t{};
    manager->register_scene<CScene>(std::string(name), vtable_copy, ctx);
}

void smlt_scene_manager_activate(smlt_scene_manager_t* self, const char* route) {
    reinterpret_cast<smlt::SceneManager*>(self)->activate(std::string(route));
}

smlt_scene_t* smlt_scene_manager_active_scene(const smlt_scene_manager_t* self) {
    auto scene = reinterpret_cast<const smlt::SceneManager*>(self)->active_scene();
    return reinterpret_cast<smlt_scene_t*>(scene.get());
}

void smlt_scene_set_user_data(smlt_scene_t* self, void* user_data) {
    auto* scene = dynamic_cast<CScene*>(reinterpret_cast<smlt::Scene*>(self));
    if(scene) {
        scene->set_user_data(user_data);
    }
}

void* smlt_scene_get_user_data(const smlt_scene_t* self) {
    auto* scene = dynamic_cast<const CScene*>(reinterpret_cast<const smlt::Scene*>(self));
    return scene ? scene->user_data() : nullptr;
}

smlt_stage_node_t* smlt_scene_as_stage_node(smlt_scene_t* self) {
    // Implicit Scene* -> StageNode* upcast, done in real C++ so the
    // compiler applies whatever base-offset adjustment Scene's multiple
    // inheritance actually needs (not assumed to be zero).
    smlt::StageNode* node = reinterpret_cast<smlt::Scene*>(self);
    return reinterpret_cast<smlt_stage_node_t*>(node);
}

} // extern "C"
