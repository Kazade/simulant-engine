// Custom logic for Smlt.Application, spliced into the generated
// ports/vala/generated/application.vala verbatim (see write_managed_classes()
// in tools/cgen/cgen_lib/vapi_emitter.py). Everything mechanical --
// run(), is_shutting_down(), scenes(), window(), etc. -- is auto-generated
// (including the Property<>-backed accessors, which the scanner
// recognizes directly); only what can't be derived from the C++ signature
// lives here: the constructor/destructor, the virtual hooks a subclass
// overrides, wrap(), and the fixed vtable trampolines bridging into them.
//
// There is at most one Application per process, created directly by user
// code (`new MyApp(config)`) and kept alive by that same local variable
// for the run's whole duration -- unlike Scene/StageNode (see their own
// custom/ snippets), nothing here needs to pin a reference across the
// FFI boundary with ref()/unref().
//
// Every hand-written method below still needs an explicit
// [CCode(cname=...)]: left to its default, valac names the C function it
// generates for e.g. `Application.run()` exactly `smlt_application_run`
// -- identical to the mechanical free function of the same name it calls
// internally, since both follow the same smlt_<class>_<method>
// convention. Without the override, that's a straight symbol collision
// at compile time. The `smlt_vala_` prefix guarantees no clash with
// anything mechanical, ever -- the auto-generated methods do the same.

protected Application(Smlt.AppConfig config) {
    Smlt.ApplicationVtable vtable = Smlt.ApplicationVtable();
    vtable.init = _dispatch_init;
    vtable.pre_init = _dispatch_pre_init;
    vtable.fixed_update = _dispatch_fixed_update;
    vtable.update = _dispatch_update;
    vtable.late_update = _dispatch_late_update;
    vtable.clean_up = _dispatch_clean_up;
    native = Smlt.application_create_custom(config, null, vtable, null);
    Smlt.application_set_user_data(native, (void*) this);
}

~Application() {
    if(native != null) {
        _c_destroy(native);
    }
}

// Virtual lifecycle hooks. Default bodies match the C vtable's
// "null callback" behavior (see application_ext.h).
[CCode (cname = "smlt_vala_application_init")]
public virtual bool init() { return true; }
[CCode (cname = "smlt_vala_application_pre_init")]
public virtual bool pre_init() { return true; }
[CCode (cname = "smlt_vala_application_fixed_update")]
public virtual void fixed_update(float dt) {}
[CCode (cname = "smlt_vala_application_update")]
public virtual void update(float dt) {}
[CCode (cname = "smlt_vala_application_late_update")]
public virtual void late_update(float dt) {}
[CCode (cname = "smlt_vala_application_clean_up")]
public virtual void clean_up() {}

// Reconstitutes the Vala wrapper pinned to a raw smlt_application_t*, if
// any -- the same role StageNode.wrap()/Scene.wrap() play for their own
// hierarchies. There's only ever one Application per process, created and
// kept alive by the user's own `main()`.
public static Application? wrap(void* app_native) {
    return app_native != null ? (Application) Smlt.application_get_user_data(app_native) : null;
}

// The process-wide current Application, if any (smlt_get_app()).
[CCode (cname = "smlt_vala_application_current")]
public static Application? current() {
    return wrap(Smlt.get_app());
}

// Fixed trampolines: every Application subclass shares these same
// C function pointers. `user_data` (set once above) always holds
// the actual Vala instance, so dispatch is just a cast + a normal
// (virtual) Vala method call -- `self.init()` resolves to
// whatever override the concrete subclass provides.
private static bool _dispatch_init(void* native_self, void* user_data) {
    unowned Application self = (Application) user_data;
    return self.init();
}

private static bool _dispatch_pre_init(void* native_self, void* user_data) {
    unowned Application self = (Application) user_data;
    return self.pre_init();
}

private static void _dispatch_fixed_update(void* native_self, float dt, void* user_data) {
    unowned Application self = (Application) user_data;
    self.fixed_update(dt);
}

private static void _dispatch_update(void* native_self, float dt, void* user_data) {
    unowned Application self = (Application) user_data;
    self.update(dt);
}

private static void _dispatch_late_update(void* native_self, float dt, void* user_data) {
    unowned Application self = (Application) user_data;
    self.late_update(dt);
}

private static void _dispatch_clean_up(void* native_self, void* user_data) {
    unowned Application self = (Application) user_data;
    self.clean_up();
}
