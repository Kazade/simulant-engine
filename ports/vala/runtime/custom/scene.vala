// Custom logic for Smlt.Scene, spliced into the generated
// ports/vala/generated/scene.vala verbatim (see write_managed_classes()
// in tools/cgen/cgen_lib/vapi_emitter.py). Everything mechanical --
// load(), is_active(), name(), etc. -- is auto-generated; only what
// can't be derived from the C++ signature lives here: the virtual hooks
// a subclass overrides, the Property<>-backed accessors the scanner
// can't see, register_scene<T>()/active(), and the fixed vtable
// trampolines bridging into all of it.
//
// Unlike Application, Scene instances are constructed by the engine, not
// directly by user code -- SceneManager reconstructs a fresh instance
// every time a route is activated (matching C++'s own
// SceneManager::register_scene<T>(name), which does the same). That
// means:
//   - Scene has no public constructor; register_scene<T>() stores the
//     GType, and the fixed _dispatch_create trampoline below calls
//     GLib.Object.new(type) itself each time a route activates.
//   - Every instance's Vala wrapper is pinned across the FFI boundary
//     with an extra ref() taken in _dispatch_create and dropped in
//     _dispatch_delete -- the latter fires from the *real* C++ destructor
//     (~CScene(), see scene_ext.cpp), not on_unload()/on_destroy(),
//     since a route can be deactivated and reactivated many times over
//     its registration's lifetime without the underlying object ever
//     actually being destroyed.

// Virtual lifecycle hooks. Default bodies match the C vtable's
// "null callback" behavior (see scene_ext.h).
[CCode (cname = "smlt_vala_scene_on_load")]
public virtual void on_load() {}
[CCode (cname = "smlt_vala_scene_on_unload")]
public virtual void on_unload() {}
[CCode (cname = "smlt_vala_scene_on_activate")]
public virtual void on_activate() {}
[CCode (cname = "smlt_vala_scene_on_deactivate")]
public virtual void on_deactivate() {}
[CCode (cname = "smlt_vala_scene_on_update")]
public virtual void on_update(float dt) {}
[CCode (cname = "smlt_vala_scene_on_fixed_update")]
public virtual void on_fixed_update(float dt) {}

// Scene::assets/compositor/input are C++ Property<> smart members, but
// they're mechanically generated now (the scanner recognizes Property<>
// fields directly -- see _try_bind_property_field() in scanner.py), so
// only as_stage_node() needs to live here: Scene *is* a StageNode in C++
// (its first base), but that relationship isn't mirrored in Vala (Scene
// is its own managed-class root, not part of the generated StageNode
// hierarchy) -- see is_scene_or_subclass in scanner.py.
[CCode (cname = "smlt_vala_scene_as_stage_node")]
public StageNode as_stage_node() { return StageNode.wrap(Smlt.scene_as_stage_node(native)); }

// Registers T (a Scene subclass with a parameterless constructor)
// under `name`, mirroring SceneManager::register_scene<T>(name)
// in C++. A fresh T is constructed every time `name` is
// activated -- see _dispatch_create below. Called via
// SceneManager.register_scene<T>(name) in practice (see
// ports/vala/runtime/custom/scene_manager.vala), not directly.
[CCode (cname = "smlt_vala_scene_register")]
public static void register_scene<T>(void* manager_native, string name) {
    SceneVtable vtable = SceneVtable();
    vtable.on_load = _dispatch_on_load;
    vtable.on_unload = _dispatch_on_unload;
    vtable.on_activate = _dispatch_on_activate;
    vtable.on_deactivate = _dispatch_on_deactivate;
    vtable.on_update = _dispatch_on_update;
    vtable.on_fixed_update = _dispatch_on_fixed_update;
    vtable.create_user_data = _dispatch_create;
    vtable.delete_user_data = _dispatch_delete;
    Type type = typeof(T);
    Smlt.scene_register_type(manager_native, name, vtable, (void*)(ulong) type);
}

// Reconstitutes the Vala wrapper pinned to a raw smlt_scene_t*, if any --
// the same role StageNode.wrap() plays for its own hierarchy. Every
// mechanically generated method elsewhere that returns a Scene (e.g.
// StageNode.scene()) calls this to convert the raw pointer back.
public static Scene? wrap(void* scene_native) {
    return scene_native != null ? (Scene) Smlt.scene_get_user_data(scene_native) : null;
}

// The currently active Scene on `manager`, if any.
[CCode (cname = "smlt_vala_scene_active")]
public static Scene? active(void* manager_native) {
    return wrap(Smlt.scene_manager_active_scene(manager_native));
}

// Fixed trampolines, shared by every Scene subclass -- see
// Application's for the general shape of this pattern.
private static void* _dispatch_create(void* native_self, void* ctx) {
    Type type = (Type)(ulong) ctx;
    var obj = (Scene) GLib.Object.new(type);
    obj.native = native_self;
    return (void*) obj.ref();
}

private static void _dispatch_delete(void* user_data) {
    ((Scene) user_data).unref();
}

private static void _dispatch_on_load(void* native_self, void* user_data) {
    ((Scene) user_data).on_load();
}

private static void _dispatch_on_unload(void* native_self, void* user_data) {
    ((Scene) user_data).on_unload();
}

private static void _dispatch_on_activate(void* native_self, void* user_data) {
    ((Scene) user_data).on_activate();
}

private static void _dispatch_on_deactivate(void* native_self, void* user_data) {
    ((Scene) user_data).on_deactivate();
}

private static void _dispatch_on_update(void* native_self, float dt, void* user_data) {
    ((Scene) user_data).on_update(dt);
}

private static void _dispatch_on_fixed_update(void* native_self, float dt, void* user_data) {
    ((Scene) user_data).on_fixed_update(dt);
}
