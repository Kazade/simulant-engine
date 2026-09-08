/* Manual bindings for ports/c/include/simulant/c/*_ext.h -- the
 * hand-written C additions that cgen.py can't produce mechanically
 * (Property<> accessors, StageNode/Scene/Application subclassing
 * vtables, built-in node construction). Unlike simulant-c.vapi, this
 * file is NOT generated and must be kept in sync by hand with the ext
 * headers under ports/c/include/simulant/c/.
 *
 * Every declaration carries its own `cheader_filename`: reopening
 * `namespace Smlt` per header (matching one ext header per block) does
 * NOT propagate a block-level cheader_filename the way it does in a
 * single-namespace file like simulant-c.vapi -- only the first block's
 * include survived, silently dropping the rest. Repeating the attribute
 * per-declaration avoids relying on that.
 *
 * `[CCode (has_target = false)]` delegates below compile to plain C
 * function pointers (no closure environment) so they can be stored
 * directly into the vtable structs' fields, matching how the C API
 * expects a `void*` and a raw callback rather than a bound method --
 * ordinary state must go through each vtable's `user_data` parameter
 * instead of instance capture.
 */

namespace Smlt {
    // `self`/return here is always the *raw* smlt_application_t* -- never
    // a Vala Application reference. Application is now a hand-written
    // real Vala class (ports/vala/runtime/application.vala, see
    // HAND_WRITTEN_VALA_CLASSES in vapi_emitter.py) wrapping this pointer
    // in its own `native` field; these declarations are its low-level
    // plumbing, not meant to be called directly.
    [CCode (has_target = false, cheader_filename = "simulant/c/application_ext.h")]
    public delegate bool ApplicationInitFunc(void* self, void* user_data);
    [CCode (has_target = false, cheader_filename = "simulant/c/application_ext.h")]
    public delegate void ApplicationUpdateFunc(void* self, float dt, void* user_data);
    [CCode (has_target = false, cheader_filename = "simulant/c/application_ext.h")]
    public delegate void ApplicationCleanUpFunc(void* self, void* user_data);

    [CCode (cname = "smlt_application_vtable_t", has_type_id = false,
            cheader_filename = "simulant/c/application_ext.h")]
    public struct ApplicationVtable {
        public ApplicationInitFunc init;
        public ApplicationInitFunc pre_init;
        public ApplicationUpdateFunc fixed_update;
        public ApplicationUpdateFunc update;
        public ApplicationUpdateFunc late_update;
        public ApplicationCleanUpFunc clean_up;
    }

    [CCode (cname = "smlt_app_config_create_default", cheader_filename = "simulant/c/application_ext.h")]
    public extern AppConfig app_config_create_default();

    [CCode (cname = "smlt_application_create_custom", cheader_filename = "simulant/c/application_ext.h")]
    public extern void* application_create_custom(AppConfig config, void* platform_state,
                                                   ApplicationVtable? vtable, void* user_data);

    [CCode (cname = "smlt_application_set_user_data", cheader_filename = "simulant/c/application_ext.h")]
    public extern void application_set_user_data(void* self, void* user_data);
    [CCode (cname = "smlt_application_get_user_data", cheader_filename = "simulant/c/application_ext.h")]
    public extern void* application_get_user_data(void* self);

    // `self`/`ctx`/return here are always the *raw* smlt_scene_t*/void* --
    // never a Vala Scene reference. Scene is now a hand-written real Vala
    // class (ports/vala/runtime/scene.vala, see HAND_WRITTEN_VALA_CLASSES
    // in vapi_emitter.py) wrapping this pointer in its own `native` field.
    [CCode (has_target = false, cheader_filename = "simulant/c/scene_ext.h")]
    public delegate void SceneLifecycleFunc(void* self, void* user_data);
    [CCode (has_target = false, cheader_filename = "simulant/c/scene_ext.h")]
    public delegate void SceneUpdateFunc(void* self, float dt, void* user_data);
    [CCode (has_target = false, cheader_filename = "simulant/c/scene_ext.h")]
    public delegate void* SceneCreateUserDataFunc(void* self, void* ctx);
    [CCode (has_target = false, cheader_filename = "simulant/c/scene_ext.h")]
    public delegate void SceneDeleteUserDataFunc(void* user_data);

    [CCode (cname = "smlt_scene_vtable_t", has_type_id = false,
            cheader_filename = "simulant/c/scene_ext.h")]
    public struct SceneVtable {
        public SceneLifecycleFunc on_load;
        public SceneLifecycleFunc on_unload;
        public SceneLifecycleFunc on_activate;
        public SceneLifecycleFunc on_deactivate;
        public SceneUpdateFunc on_update;
        public SceneUpdateFunc on_fixed_update;
        public SceneCreateUserDataFunc create_user_data;
        public SceneDeleteUserDataFunc delete_user_data;
    }

    [CCode (cname = "smlt_scene_register_type", cheader_filename = "simulant/c/scene_ext.h")]
    public extern void scene_register_type(void* scene_manager, string name,
                                           SceneVtable? vtable, void* ctx);
    [CCode (cname = "smlt_scene_manager_activate", cheader_filename = "simulant/c/scene_ext.h")]
    public extern void scene_manager_activate(void* self, string route);
    [CCode (cname = "smlt_scene_manager_active_scene", cheader_filename = "simulant/c/scene_ext.h")]
    public extern void* scene_manager_active_scene(void* self);

    [CCode (cname = "smlt_scene_set_user_data", cheader_filename = "simulant/c/scene_ext.h")]
    public extern void scene_set_user_data(void* self, void* user_data);
    [CCode (cname = "smlt_scene_get_user_data", cheader_filename = "simulant/c/scene_ext.h")]
    public extern void* scene_get_user_data(void* self);

    [CCode (cname = "smlt_scene_as_stage_node", cheader_filename = "simulant/c/scene_ext.h")]
    public extern void* scene_as_stage_node(void* self);

    // `self`/return here are always the *raw* smlt_stage_node_t*/void* --
    // never a Vala StageNode reference. StageNode (and the whole built-in
    // node hierarchy: Sprite, Camera2d, ...) are now real Vala classes
    // generated into ports/vala/generated/ plus the hand-written root in
    // ports/vala/runtime/stage_node.vala (see HAND_WRITTEN_VALA_CLASSES in
    // vapi_emitter.py), each wrapping this pointer in its own `native`
    // field inherited from StageNode.
    [CCode (has_target = false, cheader_filename = "simulant/c/stage_node_ext.h")]
    public delegate bool StageNodeOnDestroyFunc(void* self, void* user_data);
    [CCode (has_target = false, cheader_filename = "simulant/c/stage_node_ext.h")]
    public delegate void StageNodeUpdateFunc(void* self, float dt, void* user_data);
    [CCode (has_target = false, cheader_filename = "simulant/c/stage_node_ext.h")]
    public delegate void StageNodeOnParentSetFunc(void* self, void* old_parent, void* new_parent, void* user_data);
    [CCode (has_target = false, cheader_filename = "simulant/c/stage_node_ext.h")]
    public delegate void StageNodeOnDeletedFunc(void* user_data);

    [CCode (cname = "smlt_stage_node_vtable_t", has_type_id = false,
            cheader_filename = "simulant/c/stage_node_ext.h")]
    public struct StageNodeVtable {
        public StageNodeOnDestroyFunc on_destroy;
        public StageNodeUpdateFunc on_update;
        public StageNodeUpdateFunc on_fixed_update;
        public StageNodeUpdateFunc on_late_update;
        public StageNodeOnParentSetFunc on_parent_set;
        public StageNodeOnDeletedFunc on_deleted;
    }

    [CCode (cname = "smlt_stage_node_register_type", cheader_filename = "simulant/c/stage_node_ext.h")]
    public extern uint32 stage_node_register_type(void* scene, string type_name, StageNodeVtable? vtable);
    [CCode (cname = "smlt_stage_node_create_custom", cheader_filename = "simulant/c/stage_node_ext.h")]
    public extern void* stage_node_create_custom(void* scene, void* parent, uint32 type_id);
    [CCode (cname = "smlt_stage_node_set_user_data", cheader_filename = "simulant/c/stage_node_ext.h")]
    public extern void stage_node_set_user_data(void* self, void* user_data);
    [CCode (cname = "smlt_stage_node_get_user_data", cheader_filename = "simulant/c/stage_node_ext.h")]
    public extern void* stage_node_get_user_data(void* self);
    [CCode (cname = "smlt_spritesheet_attrs_create_default", cheader_filename = "simulant/c/sprite_ext.h")]
    public extern SpritesheetAttrs spritesheet_attrs_create_default();

    [CCode (cname = "smlt_input_axis_set_positive_keyboard_key_value", cheader_filename = "simulant/c/input_axis_ext.h")]
    public extern void input_axis_set_positive_keyboard_key(InputAxis self, KeyboardCode key);
    [CCode (cname = "smlt_input_axis_set_negative_keyboard_key_value", cheader_filename = "simulant/c/input_axis_ext.h")]
    public extern void input_axis_set_negative_keyboard_key(InputAxis self, KeyboardCode key);
}
