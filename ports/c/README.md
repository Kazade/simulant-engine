# ports/c: C-linkage bindings for Simulant

Builds `libsimulant-c`, a plain-C API over Simulant's public C++ classes.
See `tools/cgen/README.md` for the naming convention, the generator, and
what it can/can't wrap automatically.

## Layout

- `generated/` -- produced by `tools/cgen/cgen.py`, committed to the repo.
  Don't hand-edit; regenerate with `cmake --build . --target
  simulant-c-regenerate` (or run the script directly).
- `include/simulant/c/`, `src/` -- hand-written additions the generator
  can't produce mechanically (see below). Public API, installed/included
  the same way as the generated headers.
- `runtime/` -- internal-only helpers used by generated code (e.g. string
  ownership across the C boundary). Not part of the public API.

## Subclassing hooks (`stage_node_ext.h`, `scene_ext.h`, `application_ext.h`)

`StageNode`, `Scene`, and `Application` are designed to be *subclassed* in
C++: the engine calls virtual methods (`on_update`, `init`, `on_load`, ...)
that a subclass overrides -- `Application::init()` and `Scene::on_load()`
are pure virtual, so a C++ program literally cannot use either class
without subclassing it. C has no vtables, so the mechanical wrapper can't
offer that -- there's nothing for the generator to scan that says "these
particular virtuals are extension points" (a class can have many virtuals
and only a few are meant to be overridden). All three extensions hand-
implement the same trampoline pattern Simulant's own `LuaStageNode` uses
internally for script-defined node types: one concrete C++ shim class per
extension point forwards each overridable hook to a stored C function
pointer (or falls back to the engine default if it's NULL), plus a
`void* user_data` per instance standing in for what a C++ subclass would
keep as member state.

`stage_node_ext.h` usage:

```c
#include "simulant/c/stage_node_ext.h"

static void my_on_update(smlt_stage_node_t* self, float dt, void* user_data) {
    int* frame_count = user_data;
    (*frame_count)++;
}

smlt_stage_node_vtable_t vtable = {0};
vtable.on_update = my_on_update;

/* Once per scene, before creating instances: */
uint32_t type_id = smlt_stage_node_register_type(scene, "my_node", &vtable);

int frame_count = 0;
smlt_stage_node_t* node = smlt_stage_node_create_custom(scene, parent, type_id);
smlt_stage_node_set_user_data(node, &frame_count);
```

Crucially, `smlt_stage_node_create_custom` creates the node through
`StageNodeManager::register_stage_node`/`create_node` -- the engine's real,
manager-owned allocation path (a slab allocator, tracked in
`nodes_by_type_`/`all_nodes_`) -- which is what makes the node actually
show up in the per-frame `update()`/`fixed_update()`/`late_update()` sweep.
This was validated against a real `Application`/`Window`/`Scene` (not just
compiled): a registered type's `on_update`/`on_fixed_update` fire once per
real engine frame, `on_parent_set` reports the correct parent, and
`user_data` round-trips correctly.

### All three vtables are copied by value, not by pointer

Every `*_register_type`/`*_create_custom` function copies the
`vtable` struct it's given rather than storing the caller's pointer. This
isn't just defensive: registration and construction are often two
different points in time here (`smlt_scene_register_type` stores a
factory closure that `SceneManager` only invokes the first time that route
is *activated*, which Simulant itself defers to the next frame's
late-update; `smlt_stage_node_register_type` similarly only runs its
factory closure whenever `create_node()` is next called for that type).
The natural place to call these registration functions is from inside an
`Application::init()` or similar short-lived callback, where a
stack-local `smlt_..._vtable_t` would otherwise be long gone by the time
the engine actually needs it -- caught by writing exactly that pattern in
this directory's own validation program, which segfaulted through a
dangling vtable pointer captured by `SceneManager`'s factory closure
before the fix.

### `Application`/`Scene` (`application_ext.h`, `scene_ext.h`)

```c
#include "simulant/c/simulant_c.h"
#include "simulant/c/application_ext.h"
#include "simulant/c/scene_ext.h"

static bool my_init(smlt_application_t* self, void* user_data) {
    smlt_scene_manager_t* scenes = smlt_application_scenes(self);

    smlt_scene_vtable_t scene_vtable = {0};
    scene_vtable.on_load = my_on_load;
    scene_vtable.on_update = my_scene_update;
    smlt_scene_register_type(scenes, "main", &scene_vtable, user_data);
    smlt_scene_manager_activate(scenes, "main");
    return true;
}

int main(void) {
    smlt_app_config_t* config = smlt_app_config_create_default();

    smlt_application_vtable_t vtable = {0};
    vtable.init = my_init;
    vtable.update = my_app_update;

    smlt_application_t* app = smlt_application_create_custom(config, NULL, &vtable, NULL);
    smlt_app_config_destroy(config);

    int32_t code = smlt_application_run(app);
    smlt_application_destroy(app);
    return code;
}
```

Notes specific to these two:

- `Application` and `Scene` are both plain/shared_ptr-owned in the real
  engine (unlike `Actor`/`Camera`/..., they don't go through
  `StageNodeManager`'s slab allocator), so `smlt_application_create_custom`
  is a plain `new`, and its result is released with the ordinary
  mechanically generated `smlt_application_destroy()` -- no special
  release function needed. `Scene` instances are owned by `SceneManager`
  once activated (via `std::shared_ptr<Scene>`/`ScenePtr`) and are never
  created directly from C; only registered by name and activated.
- `smlt_app_config_create_default()` exists because `AppConfig` has no
  user-declared constructor at all, so `cgen.py` can't safely assume an
  implicit default one exists in general (see "Known limitations" in
  `tools/cgen/README.md`) -- this one case was hand-verified instead of
  reopening that generator heuristic.
- `Application::scenes`/`Application::window`, `Scene::assets`, and
  `SceneManager::activate()`/`active_scene()` aren't reachable through the
  mechanical wrapper at all: the first three are C++ `Property<>` smart
  members (a wrapper type the generator doesn't understand -- note this is
  a different thing from the `Asset`/`shared_ptr` handling below, even
  though `Scene::assets` does return something `shared_ptr`-flavored),
  `activate()` is a variadic template, and `active_scene()` returns
  `std::shared_ptr<Scene>` (specifically unsupported -- see the `Asset`
  section below for why `Scene` is deliberately excluded from the
  `shared_ptr` handle convention). `smlt_application_scenes()`,
  `smlt_application_window()`, `smlt_scene_assets()`,
  `smlt_scene_manager_activate()`, and `smlt_scene_manager_active_scene()`
  are hand-written equivalents living alongside the vtable machinery in
  these same two files.
- `Scene::on_update()`/`on_fixed_update()` aren't purely user hooks the
  way `StageNode`'s are -- Simulant's own `Scene` overrides them to drive
  per-frame service updates. The `CScene` shim always calls
  `Scene::on_update()`/`on_fixed_update()` first and only *then* invokes
  the optional vtable callback, so a caller can't accidentally break
  service updates by supplying one.

All of the above was validated the same way as `StageNode`: a real
`Application` built via `smlt_application_create_custom`, running real
frames via `smlt_application_run_frame()`, with a custom `Scene`
registered and activated from inside the C `init` callback -- confirming
`init`/`on_load`/`on_activate` each fire exactly once, `update` fires once
per frame on both the `Application` and the active `Scene`, and
`user_data` round-trips correctly on both.

### `StageNode` subclasses don't get a mechanical `create()`/`destroy()`

Building this surfaced a real bug: the *mechanically generated*
`smlt_<node>_create()`/`_destroy()` the scanner produces for other classes
would, for `StageNode`-derived classes (`Actor`, `Camera`, `Light`,
`Sprite`, ...), call the C++ constructor/destructor directly (`new`/
`delete`). That bypasses `StageNodeManager` entirely: the resulting object
would never be registered, never receive `update()`, be invisible to the
scene graph, and not be slab-allocated the way the manager expects when
nodes it *did* create are cleaned up. It would compile and run in
isolation but not be a functional way to add a live node to a scene.

`tools/cgen/cgen_lib/scanner.py` now detects (transitive) `StageNode`
subclasses via `clangutil.is_derived_from()` and skips generating
`_create()`/`_destroy()` for them entirely -- all their other instance
methods are still wrapped normally, since those work correctly regardless
of how the pointer was obtained. `smlt_stage_node_create_custom` (this
directory's `stage_node_ext.h`) is the only supported way to create a
`StageNode` from C today. Extending an equivalent manager-based creation
path to Simulant's *built-in* node types (`Actor`, `Camera`, ...) -- e.g. a
`smlt_stage_node_create_builtin(scene, parent, "actor")`-style API -- is a
natural follow-up but a separate, larger change (their construction goes
through the `NodeParam`/`get_node_params<T>()` introspection system, not a
plain constructor call).

### Assets (`Texture`, `Material`, `Mesh`, ...): `std::shared_ptr<T>` support

`Asset` is the other manager-owned base class in the engine (assets are
loaded/created through `AssetManager::load_texture()`, `create_material()`,
etc., not a bare constructor, and are reference-counted via
`std::shared_ptr<T>` -- `TexturePtr`, `MaterialPtr`, ...). The scanner's
`MANAGED_LIFETIME_BASES` check (`tools/cgen/cgen_lib/scanner.py`) detects
`smlt::Asset` subclasses and suppresses their mechanical `_create()` the
same way as `StageNode` (there's still no bare-constructor way to make a
fresh, unmanaged one), but a release function and every other `shared_ptr`
return/param involving an `Asset` subclass **are** now wrapped, mapped
through a shared-ownership handle convention distinct from every other
class in this codebase:

- A `smlt_texture_t*` (or any other `Asset`-derived handle) does **not**
  point at a `Texture` the way `smlt_vec3_t*` points at a `Vec3`. It points
  at a heap-allocated `std::shared_ptr<Texture>` *wrapper*. Every generated
  method/field access on the class dereferences that wrapper first
  (`(*reinterpret_cast<std::shared_ptr<Texture>*>(self))->method()`) and
  relies on `shared_ptr::operator->()` to reach the real object.
- Cleanup is named `smlt_texture_release()`, not `smlt_texture_destroy()`
  -- deliberately different from the `_destroy()` used everywhere else in
  this wrapper, since "destroy" already means something specific in the
  engine (e.g. `StageNode::destroy()`) and this isn't that: it deletes the
  `shared_ptr<Texture>` wrapper, dropping *this* reference, which does not
  necessarily free the underlying `Texture` if something else (e.g.
  `AssetManager`'s own cache) still holds a reference to it.
- Any method/free-function returning `std::shared_ptr<X>` (by value or
  `const X&`) for a registered `X` with this lifetime -- `X` derives from
  `smlt::Asset` -- gets a fresh heap-allocated `shared_ptr<X>` copy
  (bumping the refcount) returned as `smlt_x_t*`, e.g.
  `smlt_asset_manager_load_texture()`, `smlt_actor_mesh()`,
  `smlt_asset_manager_create_material()`. The caller owns this reference
  and must call the matching `_release()`, the same "by-value return is
  owned, caller must clean it up" convention as everywhere else in the
  wrapper, just with a name that doesn't overload "destroy".
- Symmetrically, a parameter typed `std::shared_ptr<X>`/`const X&` (e.g.
  `Actor::set_mesh(const MeshPtr&, ...)`) accepts the same `smlt_x_t*`
  handle and dereferences the wrapper to get a real `shared_ptr<X>&` to
  pass through.
- A `std::shared_ptr<Y>` where `Y` is *not* an `Asset` subclass is still
  unsupported and skipped with a warning naming `Y` -- this convention
  only turns on for the specific ownership model `Asset` actually uses; it
  isn't a general `shared_ptr` unwrapping mechanism (e.g. `Scene`'s own
  `ScenePtr = std::shared_ptr<Scene>` is deliberately *not* treated this
  way, since `Scene` derives `StageNode` and every existing
  `smlt_scene_t*` handle -- including the ones this wrapper itself already
  hands out in `stage_node_ext.h`/`scene_ext.h` -- is a raw `Scene*`;
  making `Scene` shared_ptr-backed too would silently break all of those).

This was validated against the real engine, not just compiled: loading the
same texture path twice through `AssetManager::load_texture()` (which
caches and returns two independent `shared_ptr`s to the *same* underlying
`Texture`), destroying the first handle, and confirming the second handle
is still perfectly valid and reports the correct texture dimensions --
i.e. the refcounting is actually keeping the object alive correctly, not
just avoiding an immediate crash.

Building this surfaced two more pre-existing gaps in Simulant itself
(declared methods with no implementation, in the same vein as
`Path::exists()` noted in `tools/cgen/README.md`):
`AssetManager::particle_script(AssetID) const` and
`Texture::update_palette()`. Both link-fail if a program's translation
unit actually references the generated wrapper function for them; harmless
otherwise since C++ only requires a definition for symbols actually used.
