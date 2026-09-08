// Custom logic for Smlt.StageNode -- the root of the whole generated
// node hierarchy (Sprite, Camera2d, ...) -- spliced into the generated
// ports/vala/generated/stage_node.vala verbatim (see
// write_managed_classes() in tools/cgen/cgen_lib/vapi_emitter.py).
// Everything mechanical -- first_sibling(), get_transform(), etc. -- is
// auto-generated; only what can't be derived from the C++ signature
// lives here: the wrap() cache bridging raw pointers to Vala object
// identity, the destroy() ext accessor (invisible to the scanner),
// the virtual hooks a subclass overrides, create_child<T>(), and the
// fixed vtable trampolines bridging into all of it.
//
// Two different node "kinds" exist underneath, and create_child<T>()
// dispatches between them transparently:
//
//  - Built-in types (Sprite, Camera2d, ...): real, distinct C++ classes,
//    constructed via their own generated smlt_stage_node_create_child_X()
//    (see register_builtin_factory() below). The C++ engine drives these
//    directly through its own virtual dispatch -- a Vala override of
//    on_update()/on_destroy()/etc. on a *subclass of a built-in type*
//    (e.g. `class MyBird : Sprite`) is never actually called by the
//    engine, since construction always produces a plain smlt::Sprite.
//    (This is also why the generated per-class files skip mechanically
//    wrapping Sprite::on_update() and friends -- see _reserved_names() in
//    vapi_emitter.py -- they'd collide with the hooks below anyway.)
//
//  - Custom Vala-authored types (`class MyNode : StageNode`): backed by
//    the generic CStageNode C++ shim (stage_node_ext.cpp), whose vtable
//    is a *fixed* set of trampolines shared by every custom Vala node
//    ever created, dispatching through user_data to whichever Vala
//    instance and (thanks to ordinary virtual dispatch) whichever
//    override actually applies. This is the case the vtable hooks below
//    exist for.
//
// Pinning: a custom node's Vala wrapper is ref()'d once when created and
// unref()'d from on_deleted (fired from ~CStageNode(), not on_destroy(),
// which can veto and doesn't mean the object is really gone) -- see
// scene.vala's header comment for the identical reasoning. Foreign nodes
// reached generically (wrap()'s fallback path) are cached indefinitely;
// see the caveat on the cache field below.

// StageNode::destroy() is inherited from DestroyableObject, a non-first
// base the scanner now flattens onto StageNode directly (see
// iter_inherited_public_members() in clangutil.py) -- reachable here as
// the private extern _c_stage_node_destroy, the same mechanical name the
// generated file would've used for a public method of this name had this
// class not already declared its own (see _skip_names_for() in
// vapi_emitter.py). scene_native() reuses the mechanically generated
// private extern backing the public, wrapped scene() method (StageNode::
// scene is a Property<>, recognized directly by the scanner) --
// create_child<T>() below needs the *raw* smlt_scene_t*, not a Vala Scene
// wrapper.
[CCode (cname = "smlt_stage_node_destroy", cheader_filename = "simulant/c/stage_node.h")]
private static extern bool _c_stage_node_destroy(void* self);

[CCode (cname = "smlt_vala_stage_node_destroy")]
public bool destroy() { return _c_stage_node_destroy(native); }
[CCode (cname = "smlt_vala_stage_node_scene_native")]
internal void* scene_native() { return _c_stage_node_scene(native); }

// -- user-overridable virtual hooks --------------------------------
// Only ever invoked for custom Vala-authored node types (created
// via create_child<T>() where T isn't a registered built-in) --
// see the file header comment for why a built-in subclass's
// override is never reached.

[CCode (cname = "smlt_vala_stage_node_on_update")]
public virtual void on_update(float dt) {}
[CCode (cname = "smlt_vala_stage_node_on_fixed_update")]
public virtual void on_fixed_update(float step) {}
[CCode (cname = "smlt_vala_stage_node_on_late_update")]
public virtual void on_late_update(float dt) {}
[CCode (cname = "smlt_vala_stage_node_on_destroy")]
public virtual bool on_destroy() { return true; }
[CCode (cname = "smlt_vala_stage_node_on_parent_set")]
public virtual void on_parent_set(StageNode? old_parent, StageNode? new_parent) {}

// -- construction ---------------------------------------------------

[CCode (has_target = false)]
public delegate void* NativeFactory(void* parent_native);

private static GLib.HashTable<string, Type>? _builtin_types;
private static GLib.HashTable<Type, unowned NativeFactory>? _builtin_factories;
// Foreign/built-in nodes wrap()-ed on demand, keyed by native
// pointer, holding one permanent reference each. There's no
// per-node "this is being destroyed" notification available for
// arbitrary foreign nodes (only for custom ones, via on_deleted),
// so entries are never evicted: if the engine frees a node and
// later reuses its address, a stale entry here could return the
// wrong wrapper. Same category of hazard as any other borrowed
// pointer in the mechanical API, not a new one -- see
// ports/c/README.md.
private static GLib.HashTable<void*, void*>? _wrapper_cache;
private static bool _registered = false;

internal static void register_builtin_type(string type_name, Type type) {
    if(_builtin_types == null) {
        _builtin_types = new GLib.HashTable<string, Type>(str_hash, str_equal);
    }
    _builtin_types.insert(type_name, type);
}

internal static void register_builtin_factory(Type type, NativeFactory factory) {
    if(_builtin_factories == null) {
        _builtin_factories = new GLib.HashTable<Type, unowned NativeFactory>(direct_hash, direct_equal);
    }
    _builtin_factories.insert(type, factory);
}

private static void _ensure_registered() {
    if(!_registered) {
        _registered = true;
        StageNodeRegistry.register_all();
    }
}

// Wraps a raw smlt_stage_node_t* as its correctly-typed Vala
// object -- the fast path (a custom Vala node, or one already
// seen before) is just a cast/cache lookup; a never-before-seen
// foreign/built-in node allocates a new wrapper of the right
// concrete type via node_type_name(), or a plain StageNode if
// that name isn't a registered built-in (still fully usable via
// every method above, just not further castable to e.g. Sprite).
[CCode (cname = "smlt_vala_stage_node_wrap")]
internal static StageNode? wrap(void* native) {
    if(native == null) {
        return null;
    }

    void* user_data = Smlt.stage_node_get_user_data(native);
    if(user_data != null) {
        unowned StageNode obj = (StageNode) user_data;
        return obj;
    }

    _ensure_registered();
    if(_wrapper_cache != null) {
        void* cached = _wrapper_cache.lookup(native);
        if(cached != null) {
            unowned StageNode obj = (StageNode) cached;
            return obj;
        }
    }

    string type_name = _c_stage_node_node_type_name(native);
    Type type = typeof(StageNode);
    if(_builtin_types != null) {
        Type found = _builtin_types.lookup(type_name);
        if(found != Type.INVALID) {
            type = found;
        }
    }

    var obj2 = (StageNode) GLib.Object.new(type);
    obj2.native = native;
    if(_wrapper_cache == null) {
        _wrapper_cache = new GLib.HashTable<void*, void*>(direct_hash, direct_equal);
    }
    _wrapper_cache.insert(native, (void*) obj2.ref());
    return obj2;
}

// StageNode::create_child<T>() equivalent. T must be either a
// registered built-in node type, or a plain Vala class deriving
// StageNode with a parameterless constructor.
[CCode (cname = "smlt_vala_stage_node_create_child")]
public T create_child<T>() {
    _ensure_registered();
    Type type = typeof(T);

    unowned NativeFactory? factory = _builtin_factories != null ? _builtin_factories.lookup(type) : null;
    if(factory != null) {
        void* child_native = factory(native);
        return (T) StageNode.wrap(child_native);
    }

    // Custom Vala-authored node type: backed by the generic
    // CStageNode shim (see stage_node_ext.cpp). Re-registering the
    // same type name is a cheap no-op on the engine side (see
    // smlt_stage_node_register_type()'s doc comment), so no extra
    // bookkeeping is needed here to register only once.
    void* scene_native = this.scene_native();
    StageNodeVtable vtable = StageNodeVtable();
    vtable.on_destroy = _dispatch_on_destroy;
    vtable.on_update = _dispatch_on_update;
    vtable.on_fixed_update = _dispatch_on_fixed_update;
    vtable.on_late_update = _dispatch_on_late_update;
    vtable.on_parent_set = _dispatch_on_parent_set;
    vtable.on_deleted = _dispatch_on_deleted;
    uint32 type_id = Smlt.stage_node_register_type(scene_native, type.name(), vtable);
    void* child_native = Smlt.stage_node_create_custom(scene_native, native, type_id);

    var obj = (StageNode) GLib.Object.new(type);
    obj.native = child_native;
    Smlt.stage_node_set_user_data(child_native, (void*) obj.ref());
    return (T) obj;
}

// Fixed trampolines shared by every custom Vala node type -- see
// the file header comment.
private static bool _dispatch_on_destroy(void* native_self, void* user_data) {
    return ((StageNode) user_data).on_destroy();
}

private static void _dispatch_on_update(void* native_self, float dt, void* user_data) {
    ((StageNode) user_data).on_update(dt);
}

private static void _dispatch_on_fixed_update(void* native_self, float step, void* user_data) {
    ((StageNode) user_data).on_fixed_update(step);
}

private static void _dispatch_on_late_update(void* native_self, float dt, void* user_data) {
    ((StageNode) user_data).on_late_update(dt);
}

private static void _dispatch_on_parent_set(void* native_self, void* old_parent, void* new_parent, void* user_data) {
    ((StageNode) user_data).on_parent_set(StageNode.wrap(old_parent), StageNode.wrap(new_parent));
}

private static void _dispatch_on_deleted(void* user_data) {
    ((StageNode) user_data).unref();
}
