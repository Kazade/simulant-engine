#pragma once

/* Lets C code define its own Scene "subclass", the same way stage_node_ext.h
 * does for StageNode: a vtable of callbacks invoked at the same points a
 * C++ subclass would override (on_load/on_activate/...), plus a per-scene
 * user_data pointer.
 *
 * Scene::on_load() is a pure virtual in C++ -- vtable.on_load is the only
 * callback that's meaningfully required; a NULL on_load just does nothing.
 *
 * Unlike smlt_stage_node_create_custom(), scenes are registered by *name*
 * (matching SceneManager::register_scene<T>("route")) and instantiated
 * lazily the first time that route is activated -- there's no separate
 * smlt_scene_create_custom(); use smlt_scene_manager_activate() and read
 * the result back with smlt_scene_manager_active_scene().
 *
 * Usage:
 *
 *   smlt_scene_vtable_t vtable = {0};
 *   vtable.on_load = my_on_load;
 *   vtable.on_update = my_on_update;
 *
 *   smlt_scene_manager_t* scenes = smlt_application_scenes(app);
 *   smlt_scene_register_type(scenes, "main", &vtable, my_state);
 *   smlt_scene_manager_activate(scenes, "main");
 */

#include "simulant/c/types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    void (*on_load)(smlt_scene_t* self, void* user_data);
    void (*on_unload)(smlt_scene_t* self, void* user_data);
    void (*on_activate)(smlt_scene_t* self, void* user_data);
    void (*on_deactivate)(smlt_scene_t* self, void* user_data);

    /* Called after the engine's own per-frame service/scene-tree update
     * (Scene::on_update()/on_fixed_update()), not instead of it. */
    void (*on_update)(smlt_scene_t* self, float dt, void* user_data);
    void (*on_fixed_update)(smlt_scene_t* self, float step, void* user_data);
} smlt_scene_vtable_t;

/* Registers a new custom scene type under `name` on `scene_manager`
 * (see smlt_application_scenes()). `vtable` is copied by value at
 * registration time -- it does not need to outlive this call, even though
 * instances aren't actually constructed until `name` is first activated
 * (a temporary/stack-local vtable built inside an init() callback is
 * fine). `user_data` is likewise captured at registration time and handed
 * to every instance activated under this name (mirroring how
 * SceneManager::register_scene<T>(name, args...) forwards args to every
 * instance it constructs). Registering the same name twice is a no-op
 * that keeps the first registration, logged as a warning by the engine. */
void smlt_scene_register_type(smlt_scene_manager_t* scene_manager, const char* name,
                              const smlt_scene_vtable_t* vtable, void* user_data);

/* SceneManager::activate<Args...>() is a template cgen.py can't wrap;
 * this is the zero-extra-args case, which is what registration via
 * smlt_scene_register_type() above expects. */
void smlt_scene_manager_activate(smlt_scene_manager_t* self, const char* route);

/* SceneManager::active_scene() returns a std::shared_ptr<Scene> the
 * generator can't map (see ports/c/README.md); this returns the borrowed
 * raw pointer -- do not destroy it. NULL if no scene is active. */
smlt_scene_t* smlt_scene_manager_active_scene(const smlt_scene_manager_t* self);

/* Only meaningful for scenes created via smlt_scene_register_type();
 * harmless no-op/NULL otherwise. */
void smlt_scene_set_user_data(smlt_scene_t* self, void* user_data);
void* smlt_scene_get_user_data(const smlt_scene_t* self);

/* Scene::assets is a C++ Property<> smart member (see
 * smlt_application_scenes() in application_ext.h for the same situation
 * on Application); this is the hand-written equivalent. Borrowed -- do
 * not destroy. Use it to load/create textures, materials, meshes, etc.
 * (see asset_manager.h and the *Ptr-returning methods on the relevant
 * asset classes, e.g. texture.h, material.h). */
smlt_asset_manager_t* smlt_scene_assets(smlt_scene_t* self);

#ifdef __cplusplus
}
#endif
