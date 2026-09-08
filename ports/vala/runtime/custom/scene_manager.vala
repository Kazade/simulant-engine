// Custom logic for Smlt.SceneManager, spliced into the generated
// ports/vala/generated/scene_manager.vala verbatim (see
// write_managed_classes() in tools/cgen/cgen_lib/vapi_emitter.py).
// Everything mechanical -- has_scene(), unload(), etc. -- is auto-
// generated; only the constructor/destructor and the scene_ext.h-backed
// methods (invisible to the scanner, since SceneManager::activate<Args...>
// is templated and register_scene<T>()/active_scene() don't exist in C++
// at all the way this file exposes them) live here.
//
// Unlike Application/Scene/StageNode, SceneManager isn't subclassed by
// user code -- it's hand-written purely so register_scene<T>()/
// active_scene() can be real *instance methods*, reading the same way as
// C++'s SceneManager::register_scene<T>(name): `scenes.register_scene
// <Game>("main")`, not `Scene.register_scene<Game>(scenes, "main")`.
//
// Every Application owns exactly one SceneManager, reached via
// Application.scenes() -- borrowed, so unlike a directly-constructed
// SceneManager (via `new SceneManager(window)`, mechanically valid but
// not the normal way to get one), a borrowed wrapper must never destroy
// the underlying native manager itself. `owns_native` tracks which case
// applies.

private bool owns_native = false;

[CCode (cname = "smlt_vala_scene_manager_new")]
public SceneManager(Window window) {
    native = _c_create(window);
    owns_native = true;
}

private SceneManager.borrowed(void* native) {
    this.native = native;
    this.owns_native = false;
}

// Every mechanically generated method elsewhere that returns a
// SceneManager (e.g. Application.scenes()) calls this to convert the raw
// pointer back -- not public, since a fresh wrapper each call is an
// internal implementation detail (SceneManager has no state of its own
// besides `native`, so this is cheap and identity doesn't matter the way
// it does for StageNode.wrap()). Named to match StageNode.wrap()/
// Scene.wrap()/Application.wrap() -- the generator looks up a managed
// type's wrap() method by this exact name (see _wrap_root_for() in
// vapi_emitter.py).
internal static SceneManager? wrap(void* native) {
    return native != null ? new SceneManager.borrowed(native) : null;
}

~SceneManager() {
    if(owns_native && native != null) {
        _c_destroy(native);
    }
}

// scene_ext.h-backed methods, matching C++'s own ergonomics.
[CCode (cname = "smlt_vala_scene_manager_activate")]
public void activate(string route) { Smlt.scene_manager_activate(native, route); }
[CCode (cname = "smlt_vala_scene_manager_register_scene")]
public void register_scene<T>(string name) { Scene.register_scene<T>(native, name); }
[CCode (cname = "smlt_vala_scene_manager_active_scene")]
public Scene? active_scene() { return Scene.active(native); }
