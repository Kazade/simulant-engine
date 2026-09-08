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

    /* Optional. If set, called once right after construction of *each*
     * instance activated under this route (before on_load()), and its
     * return value becomes that instance's user_data (instead of `ctx`,
     * see smlt_scene_register_type() below, directly) -- lets a caller
     * that wants fresh per-activation state (e.g. a language binding
     * constructing a new managed object each time, mirroring C++'s own
     * register_scene<T>(name) constructing a fresh T() per activation)
     * get it, without every plain-C caller needing to care. `self` is
     * already valid as an opaque handle at this point (its address is
     * fixed), even though construction isn't complete -- do not call
     * anything on it yet beyond storing the pointer. NULL means "use
     * `ctx` itself as user_data", matching the older behavior of a
     * single fixed value shared by every activation. */
    void* (*create_user_data)(smlt_scene_t* self, void* ctx);

    /* Optional. Called exactly once, right before this instance's memory
     * is actually reclaimed -- NOT the same moment as on_destroy()/
     * on_unload(), which can run without the object being torn down
     * (Scene reload keeps a route registered across many load/unload
     * cycles). This is the correct place for a language binding to
     * release a reference it pinned in create_user_data(); doing it in
     * on_unload() instead would be wrong the moment a route is
     * deactivated and later reactivated without ever being destroyed. */
    void (*delete_user_data)(void* user_data);
} smlt_scene_vtable_t;

/* Registers a new custom scene type under `name` on `scene_manager`
 * (see smlt_application_scenes()). `vtable` is copied by value at
 * registration time -- it does not need to outlive this call, even though
 * instances aren't actually constructed until `name` is first activated
 * (a temporary/stack-local vtable built inside an init() callback is
 * fine). `ctx` is likewise captured at registration time; what happens
 * with it per-instance depends on vtable->create_user_data (see above).
 * Registering the same name twice is a no-op that keeps the first
 * registration, logged as a warning by the engine. */
void smlt_scene_register_type(smlt_scene_manager_t* scene_manager, const char* name,
                              const smlt_scene_vtable_t* vtable, void* ctx);

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

/* Scene::assets/compositor/input are C++ Property<> smart members --
 * the generator's scanner now recognizes Property<> fields directly and
 * mechanically wraps them (see smlt_scene_assets()/_compositor()/_input()
 * in the generated scene.h), so no hand-written equivalent lives here
 * anymore.
 *
 * Scene is-a StageNode (its first base class in C++), so it can be used
 * anywhere a generic StageNode is expected -- e.g. as the `subtree`
 * argument to smlt_scene_compositor_create_layer(), or as the `parent`
 * argument to the create_child helpers in stage_node_ext.h. Borrowed --
 * do not destroy the result separately from `self`. */
smlt_stage_node_t* smlt_scene_as_stage_node(smlt_scene_t* self);

#ifdef __cplusplus
}
#endif
