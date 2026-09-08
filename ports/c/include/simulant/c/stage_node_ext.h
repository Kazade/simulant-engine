#pragma once

/* Lets C code define its own StageNode "subclass": a set of callbacks
 * (a vtable) invoked by the engine at the same points a C++ subclass would
 * override, plus a per-instance user_data pointer standing in for what a
 * C++ subclass would normally keep as member state.
 *
 * Usage:
 *
 *   smlt_stage_node_vtable_t vtable = {0};
 *   vtable.on_update = my_on_update;
 *
 *   // Once per scene, before creating any instances in it:
 *   uint32_t type_id = smlt_stage_node_register_type(scene, "my_node", &vtable);
 *
 *   smlt_stage_node_t* node = smlt_stage_node_create_custom(scene, parent, type_id);
 *   smlt_stage_node_set_user_data(node, my_state);
 *
 * Registration is per-Scene (it mirrors Simulant's own StageNodeManager,
 * which every Scene owns independently) and per-type-name: registering the
 * same name twice on the same scene is a no-op that keeps the first
 * vtable, logged as a warning by the engine.
 */

#include "simulant/c/types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    /* Called when the node is about to be destroyed. Return false to veto
     * the destroy (matches StageNode::on_destroy's meaning); NULL means
     * "always allow". */
    bool (*on_destroy)(smlt_stage_node_t* self, void* user_data);

    void (*on_update)(smlt_stage_node_t* self, float dt, void* user_data);
    void (*on_fixed_update)(smlt_stage_node_t* self, float step, void* user_data);
    void (*on_late_update)(smlt_stage_node_t* self, float dt, void* user_data);

    /* old_parent/new_parent are NULL when the node is being detached/had no
     * previous parent, otherwise a borrowed pointer -- do not destroy it. */
    void (*on_parent_set)(smlt_stage_node_t* self, const smlt_stage_node_t* old_parent,
                          const smlt_stage_node_t* new_parent, void* user_data);

    /* Optional. Called exactly once, right before this instance's memory
     * is actually reclaimed -- NOT the same moment as on_destroy(), which
     * can veto the destroy and doesn't mean the object is really gone.
     * Lets a language binding release a reference it pinned when it set
     * user_data (see smlt_stage_node_set_user_data() below), the same
     * reasoning as smlt_scene_vtable_t's delete_user_data. */
    void (*on_deleted)(void* user_data);
} smlt_stage_node_vtable_t;

/* Registers a new custom stage node type on `scene`, named `type_name`
 * (used for StageNode::node_type_name() and to derive the returned type
 * id). `vtable` is copied by value at registration time -- it does not
 * need to outlive this call (a temporary/stack-local vtable is fine, even
 * though instances of this type are constructed later, e.g. from a
 * closure StageNodeManager invokes on demand). Any NULL callback simply
 * falls back to the engine's default (usually a no-op). */
uint32_t smlt_stage_node_register_type(smlt_scene_t* scene, const char* type_name,
                                       const smlt_stage_node_vtable_t* vtable);

/* Creates an instance of a type previously registered with
 * smlt_stage_node_register_type() on the same scene. If `parent` is
 * non-NULL the new node is attached under it, otherwise it's left
 * unparented (matching StageNode::create_child() semantics for isolated
 * root nodes). Returns NULL if `type_id` wasn't registered on this scene. */
smlt_stage_node_t* smlt_stage_node_create_custom(smlt_scene_t* scene, smlt_stage_node_t* parent,
                                                 uint32_t type_id);

/* user_data is only meaningful for nodes created via
 * smlt_stage_node_create_custom(); calling these on an ordinary node is
 * harmless but has no effect / always returns NULL. */
void smlt_stage_node_set_user_data(smlt_stage_node_t* self, void* user_data);
void* smlt_stage_node_get_user_data(const smlt_stage_node_t* self);

/* StageNode::destroy() is inherited from DestroyableObject, a non-first
 * base -- the generator's scanner now flattens public methods from any
 * such base onto the derived class directly (see
 * iter_inherited_public_members() in clangutil.py), so smlt_stage_node_
 * destroy() in the generated stage_node.h is that flattened method, not a
 * synthesized `delete` (StageNode's manager-owned lifetime suppresses
 * that -- see MANAGED_LIFETIME_BASES in scanner.py). This is the correct
 * way to release a StageNode (it goes through the owning scene's node
 * manager via DestroyableObject::destroy()'s virtual finalize_destroy());
 * never call smlt_destroyable_object_destroy() on one, which *is* the
 * synthesized `delete` for a bare DestroyableObject and would corrupt the
 * manager's pool. */

/* StageNode::scene is a C++ Property<> smart member; the generator's
 * scanner recognizes Property<> fields directly and mechanically wraps
 * them (see smlt_stage_node_scene() in the generated stage_node.h), so no
 * hand-written equivalent lives here. */

#ifdef __cplusplus
}
#endif
