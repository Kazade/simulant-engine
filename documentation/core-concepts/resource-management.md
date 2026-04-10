# Resource Management

This document explains how Simulant manages the lifecycle of every object you create -- meshes, textures, materials, actors, cameras, and more. Understanding these patterns is essential to writing correct, leak-free, and deadlock-free code.

**Related documentation:**
- [Asset Managers](../assets/asset-managers.md) -- Deep dive into asset loading, garbage collection, and manager hierarchy
- [Actors](actors.md) -- The Actor StageNode and its relationship to meshes

---

## 1. The ID System

Every resource in Simulant is identified by a typed ID. IDs are lightweight `uint64_t` values that uniquely identify an object within its manager.

### Core ID Types

| ID Type | Underlying Type | Identifies |
|---------|----------------|------------|
| `AssetID` | `uint64_t` | Meshes, textures, materials, sounds, fonts, particle scripts, binaries, prefabs |
| `StageNodeID` | `uint64_t` | Actors, cameras, lights, stages, widgets, and all StageNode subclasses |
| `GPUProgramID` | `uint32_t` | GPU shader programs |
| `ProcessID` | `uint32_t` | OS processes |

Asset IDs carry type information embedded in their upper bits. The engine provides `asset_id_matches_type(id, type)` to validate that an ID matches the expected asset type. Similarly, `stage_node_id_matches_type(id, node_type)` validates StageNode IDs.

### How IDs Are Generated

IDs are auto-incrementing counters scoped per object type:

```cpp
AssetID mesh_id = stage->assets->new_mesh();       // Returns a new unique AssetID
StageNodeID actor_id = stage->new_actor();          // Returns a new unique StageNodeID
```

You never construct IDs manually. Always use the factory methods provided by the manager.

### IDs Are Cheap to Copy and Store

Unlike smart pointers, IDs are plain integers. They are trivially copyable, storable in containers, serializable, and safe to hold as class members without affecting object lifetimes:

```cpp
class Player {
public:
    smlt::ActorID actor_id_;     // Safe to store -- no ownership implied
    smlt::MeshID mesh_id_;       // Same here
    smlt::MaterialID material_id_;
};
```

---

## 2. Why IDs Instead of Raw Pointers

Simulant uses IDs as the primary way to reference objects rather than raw or smart pointers. This design solves several problems:

### Problem 1: Dangling Pointers

If you hold a raw pointer to an object that gets destroyed, dereferencing it is undefined behavior. With IDs, you can always check whether the object still exists before accessing it:

```cpp
// With a pointer -- dangerous
Actor* actor = stage->actor(id);
// ... time passes, someone calls actor->destroy() ...
actor->move_to(1, 2, 3);  // CRASH: dangling pointer!

// With an ID -- safe
ActorID id = stage->new_actor();
// ... time passes ...
if (stage->has_actor(id)) {
    ActorPtr actor = stage->actor(id);
    actor->move_to(1, 2, 3);  // Safe: we verified it exists
}
```

### Problem 2: Ambiguous Ownership

When you hold a `shared_ptr`, you are keeping the object alive. If multiple systems hold `shared_ptr`s to the same asset, none of them can tell when it is "their" asset versus a shared one. IDs make ownership explicit: **the manager that created the object retains ownership**. You borrow access temporarily via the ID.

### Problem 3: Thread Safety

`shared_ptr` reference counting is not always thread-safe in Simulant's implementation. IDs avoid the need for atomic ref-count manipulation on every access. The manager itself handles synchronization when you exchange an ID for a pointer.

### The Golden Rule

> **Store IDs. Borrow pointers.** Keep IDs in your classes and data structures. Only resolve them to pointers when you need to use the object, and release those pointers as soon as you are done.

---

## 3. Reference Counting for Assets

All asset types in Simulant are reference-counted using `std::shared_ptr`. The `RefCounted<T>` base class (in `simulant/generic/managed.h`) enables this pattern:

```cpp
class Mesh :
    public RefCounted<Mesh>,
    public generic::Identifiable<AssetID>,
    /* ... */
{
    // ...
};
```

### How Ref Counting Works

When you load or create an asset, the manager returns a `std::shared_ptr` (e.g., `MeshPtr`, `TexturePtr`, `MaterialPtr`). Each copy of that pointer increments the reference count. When the last `shared_ptr` is destroyed or reset, the reference count drops to one (the manager's internal copy).

At that point, the garbage collector decides what happens (see [Garbage Collection](#10-garbage-collection) below).

### Automatic Reference Tracking Between Assets

Assets can reference each other. A `Mesh` references a `Material`, which in turn references `Texture`s. When you assign a texture to a material, the material increments the texture's reference count automatically:

```cpp
auto texture = assets->load_texture("brick.png");
auto material = assets->create_material();
material->set_texture(texture);  // Texture ref count increases

texture.reset();  // Texture ref count decreases, but material still holds it
// Texture is NOT garbage collected because the material references it
```

### Checking Reference Counts

You can inspect the reference count of an asset via `use_count()` on the shared pointer:

```cpp
auto mesh = assets->load_mesh("cube.obj");
S_INFO("Mesh ref count: {}", mesh.use_count());
```

A ref count of `1` means only the manager holds a reference -- the asset is eligible for garbage collection.

---

## 4. StageNodes vs Assets: Different Ownership Models

This is one of the most important distinctions in Simulant. Assets and StageNodes have fundamentally different ownership models:

### Assets: Reference-Counted

| Property | Behavior |
|----------|----------|
| Ownership | Shared (`std::shared_ptr`) |
| Deletion | Automatic via garbage collection |
| What to store | **IDs**, not pointers |
| Creation | `assets->create_mesh()`, `assets->load_texture()`, etc. |
| Destruction | Implicit when all references are dropped + GC runs |

```cpp
// Asset: reference-counted
MeshID mid = assets->create_mesh(spec);
// The manager owns the mesh. You should store the ID, not the MeshPtr.
// If you store a MeshPtr and never release it, the mesh will never be freed.
```

### StageNodes: Manager-Owned (Not Reference-Counted)

| Property | Behavior |
|----------|----------|
| Ownership | Scene/Stage owns the node (pool-allocated) |
| Deletion | Explicit via `destroy()` or `destroy_actor()` |
| What to store | **Pointers are OK**, but IDs are safer |
| Creation | `create_child<Actor>()`, `scene->create_node<Camera3D>()`, etc. |
| Destruction | Explicit -- nodes are destroyed at a safe point in the frame |

```cpp
// StageNode: NOT reference-counted
ActorID actor_id = stage->new_actor();
// The Scene owns this actor. You can store a pointer safely IF the actor
// is guaranteed to outlive your usage. But storing the ID is safer because
// you can check has_actor() before use.
```

### Key Implication

If you store a pointer to an **Asset**, the reference count will never reach zero and the asset will never be freed. This is why you should always store asset **IDs** and only resolve them to pointers when needed:

```cpp
// BAD: Holding a MeshPtr as a member prevents garbage collection
class MyClass {
    MeshPtr mesh_;  // Reference count never drops!
};

// GOOD: Store the ID instead
class MyClass {
    MeshID mesh_id_;  // No ownership implied, no GC interference

    void use_mesh(AssetManager* assets) {
        auto mesh = assets->mesh(mesh_id_);  // Borrow temporarily
        mesh->do_something();
    }  // Pointer released at end of scope
};
```

For StageNodes, storing pointers is acceptable because they are not reference-counted. However, using IDs plus existence checks (`has_actor()`) is the defensive pattern:

```cpp
StageNodeID id = some_node_id;
if (stage->has_node(id)) {
    StageNode* node = stage->find_descendent_with_id(id);
    node->transform->set_position(1, 2, 3);
}
```

---

## 5. Asset Managers: Scene-Local vs Shared

Simulant has two levels of asset manager. Understanding the difference determines whether your assets survive scene transitions or get cleaned up.

### Scene Asset Manager (`scene->assets`)

Every `Scene` owns its own `LocalAssetManager`. Assets loaded through this manager are **tied to the scene's lifecycle**:

```cpp
void GameScene::on_load() {
    // These assets belong to this scene
    auto level_mesh = assets->load_mesh("levels/forest.glb");
    auto level_music = assets->load_sound("music/forest.ogg");
    auto level_tex = assets->load_texture("textures/ground.png");
}

void GameScene::on_unload() {
    // When the scene unloads, all these assets are released automatically
}
```

Use `scene->assets` for:
- Level-specific meshes and textures
- Scene-specific sounds and music
- Materials used only in this scene

### Shared Asset Manager (`window->shared_assets` or `application->shared_assets`)

The `SharedAssetManager` lives on the `Application` and is accessible via `window->shared_assets`. Assets loaded through this manager **persist across all scenes** and are never automatically released:

```cpp
void GameScene::on_load() {
    // These assets are shared globally
    auto ui_font = window->shared_assets->load_font("fonts/Orbitron.ttf");
    auto ui_tex = window->shared_assets->load_texture("ui/button.png");
    auto click_sound = window->shared_assets->load_sound("sounds/click.ogg");
}
```

Use `window->shared_assets` for:
- UI fonts and textures
- Common sounds (clicks, UI feedback)
- Default/shared materials
- Any asset needed by multiple scenes

### Manager Hierarchy

The `AssetManager` class supports a parent-child hierarchy. The `LocalAssetManager` has the `SharedAssetManager` as its parent. When you look up an asset by ID, the manager first checks its own registry, then falls through to the parent:

```
SharedAssetManager (application->shared_assets)
  |-- LocalAssetManager (scene_a->assets)
  |-- LocalAssetManager (scene_b->assets)
  |-- LocalAssetManager (scene_c->assets)
```

This means assets loaded through `shared_assets` are visible from any scene's asset manager:

```cpp
// In Scene A:
auto shared_tex = window->shared_assets->load_texture("shared.png");

// In Scene B, you can still access it if you have the ID
TexturePtr tex = assets->texture(shared_tex->id());  // Falls through to parent
```

### The `AssetManager` API

Both managers expose the same API. For each asset type, there are these methods:

```cpp
// Loading from file
MeshPtr mesh = assets->load_mesh("model.obj");
TexturePtr tex = assets->load_texture("image.png");
MaterialPtr mat = assets->load_material("my_material.material");
SoundPtr snd = assets->load_sound("audio.ogg");
FontPtr font = assets->load_font("font.ttf");

// Creating programmatically
MeshPtr mesh = assets->create_mesh(vertex_spec);
TexturePtr tex = assets->create_texture(width, height, format);
MaterialPtr mat = assets->create_material();

// Looking up by ID
MeshPtr mesh = assets->mesh(mesh_id);
TexturePtr tex = assets->texture(tex_id);
MaterialPtr mat = assets->material(mat_id);

// Checking existence
bool exists = assets->has_mesh(mesh_id);
bool exists = assets->has_texture(tex_id);
bool exists = assets->has_material(mat_id);

// Finding by name
MeshPtr mesh = assets->find_mesh("MyMesh");
MaterialPtr mat = assets->find_material("MyMaterial");

// Counting
size_t count = assets->mesh_count();
size_t count = assets->texture_count();

// Destroying explicitly
assets->destroy_mesh(mesh_id);
assets->destroy_texture(tex_id);
assets->destroy_material(mat_id);
```

---

## 6. Getting Pointers from IDs

To use an asset, you exchange its ID for a pointer through the manager. There are two access patterns:

### Through the AssetManager Directly

```cpp
MeshID mesh_id = /* ... */;
MeshPtr mesh = stage->assets->mesh(mesh_id);
mesh->new_submesh_as_rectangle("quad", material, 1.0f, 1.0f);
```

### Through the Stage Convenience Methods

The `Stage` class provides shorthand methods that forward to its asset manager:

```cpp
MeshID mesh_id = /* ... */;
MeshPtr mesh = stage->mesh(mesh_id);  // Convenience for stage->assets->mesh()
```

### Both const and Non-const Overloads

```cpp
MeshPtr mesh = assets->mesh(id);            // Mutable access
const MeshPtr mesh = assets->mesh(id);      // Read-only access (const method)
```

### What Happens If the ID Is Invalid?

If you request a pointer for an ID that does not exist (already destroyed, or never created), you get a null `shared_ptr`:

```cpp
MeshID bogus_id = AssetID(999999);
MeshPtr mesh = assets->mesh(bogus_id);
if (!mesh) {
    S_ERROR("Mesh not found!");
}
```

This is why you should always pair pointer lookups with existence checks.

---

## 7. Checking if Resources Still Exist

Every manager provides `has_X(id)` methods to verify that a resource is still alive before you try to use it:

```cpp
// Assets
if (assets->has_mesh(mesh_id)) {
    MeshPtr mesh = assets->mesh(mesh_id);
    // Safe to use
}

if (assets->has_texture(tex_id)) { /* ... */ }
if (assets->has_material(mat_id)) { /* ... */ }
if (assets->has_sound(sound_id)) { /* ... */ }
if (assets->has_font(font_id)) { /* ... */ }
if (assets->has_particle_script(script_id)) { /* ... */ }
if (assets->has_prefab(prefab_id)) { /* ... */ }
if (assets->has_binary(bin_id)) { /* ... */ }

// StageNodes (via Stage/Scene)
if (stage->has_node(actor_id)) {
    StageNode* actor = stage->find_descendent_with_id(actor_id);
    // Safe to use
}
```

### Actor-Specific Methods

`Actor` also provides `has_mesh(DetailLevel)` to check if it has a mesh attached at a specific detail level:

```cpp
if (actor->has_mesh(DETAIL_LEVEL_NEAREST)) {
    auto mesh = actor->base_mesh();
    // Actor has a base mesh
}
```

### Why Check Before Accessing?

StageNodes can be destroyed at any time by game logic. Even though destruction is deferred to a safe point in the frame, holding a pointer to a node that has been marked for destruction will give you access to a partially-invalid object. The `has_X` pattern prevents this:

```cpp
// BAD: Holding a pointer across frames
Actor* enemy = get_enemy();
// ... several frames later ...
enemy->take_damage(10);  // Enemy may have been destroyed!

// GOOD: Store the ID and check each frame
ActorID enemy_id = get_enemy_id();
if (stage->has_node(enemy_id)) {
    StageNode* enemy = stage->find_descendent_with_id(enemy_id);
    static_cast<Actor*>(enemy)->take_damage(10);
}
```

---

## 8. Safe Access Patterns

### Pattern 1: Scope Blocks (Recommended)

Wrap asset access in curly braces so the pointer is released at the closing brace:

```cpp
{
    auto mesh = stage->assets->mesh(mesh_id);
    mesh->new_submesh_as_rectangle("floor", material, 10.0f, 10.0f);
    // mesh is released here
}
// Do other work that does not need the mesh
```

This is the **most important pattern** in Simulant resource management. It ensures locks are held for the minimum possible time.

### Pattern 2: Single-Line Access

When you only need to call one method, chain the lookup and the call:

```cpp
stage->assets->mesh(mesh_id)->recalculate_normals();
```

The temporary `shared_ptr` is released immediately after the statement.

### Pattern 3: Existence Guard

When you are not certain the resource exists:

```cpp
if (assets->has_texture(tex_id)) {
    auto tex = assets->texture(tex_id);
    material->set_diffuse_map(tex);
}
```

### Pattern 4: Iterating All Assets

When you need to operate on every asset of a type:

```cpp
assets->each_mesh([&](uint32_t index, MeshPtr mesh) {
    S_INFO("Mesh {0}: {1}", index, mesh->name());
    mesh->recalculate_aabb();
});
```

The `each_X` methods iterate all managed assets and provide both an index and a pointer.

---

## 9. Releasing References ASAP (Avoiding Deadlocks)

Simulant's asset managers use internal locking to ensure thread-safe access. If you hold a pointer to an asset for too long, you risk deadlocking when another thread (or even the same thread) tries to access the same asset through the manager.

### The Problem

```cpp
// BAD: Holding a pointer across a large block of code
auto mesh = assets->mesh(mesh_id);

// Lots of unrelated work...
do_expensive_calculation();
update_physics();
process_input();

// Still holding mesh pointer...
// Meanwhile, another system calls assets->mesh(mesh_id) and blocks!
mesh->do_something();  // Finally releases
```

### The Solution: Scope Blocks

```cpp
{
    auto mesh = assets->mesh(mesh_id);
    mesh->do_something();
}  // Released immediately

do_expensive_calculation();
update_physics();
process_input();
```

### The Solution: Single-Line Access

```cpp
assets->mesh(mesh_id)->do_something();  // Released at end of statement
```

### The Solution: References

If you need the asset for multiple calls but want to minimize the pointer's scope:

```cpp
void process_mesh(Mesh& mesh) {
    mesh.do_thing_one();
    mesh.do_thing_two();
}

{
    auto ptr = assets->mesh(mesh_id);
    process_mesh(*ptr);
}  // ptr released here
```

### Never Hold Asset Pointers Across Frames

The most common cause of deadlocks is storing asset pointers as class members:

```cpp
// NEVER do this
class Renderer {
    MeshPtr persistent_mesh_;  // Lock held indefinitely!

    void render() {
        persistent_mesh_->draw();
    }
};

// DO this instead
class Renderer {
    MeshID mesh_id_;

    void render() {
        auto mesh = assets->mesh(mesh_id_);
        mesh->draw();
    }  // Released every frame
};
```

---

## 10. Garbage Collection

Assets are automatically cleaned up by the garbage collector. There are two collection methods:

### `GARBAGE_COLLECT_PERIODIC` (Default)

The asset will be deleted at some point after the final reference is released. The manager's `update()` method checks for assets whose ref count has dropped to 1 (only the manager's internal `shared_ptr` remains) and removes them:

```cpp
{
    auto mesh = assets->load_mesh("temp.obj");
    // mesh ref count: 2 (you + manager)
} // mesh goes out of scope, ref count: 1 (manager only)

// Later, when the manager runs garbage collection:
assets->run_garbage_collection();  // mesh is deleted
```

Garbage collection runs automatically during the engine's update cycle, but you can also trigger it manually.

### `GARBAGE_COLLECT_NEVER`

The asset is kept indefinitely until you explicitly change its collection method or destroy it:

```cpp
// Load a texture that should persist across scenes
auto ui_tex = assets->load_texture(
    "ui/common.png",
    GARBAGE_COLLECT_NEVER
);

// Later, when you are truly done with it:
ui_tex->set_garbage_collection_method(GARBAGE_COLLECT_PERIODIC);
// It will now be collected once all references are released
```

### When to Use Each

| Scenario | Method |
|----------|--------|
| Level-specific mesh/texture | `GARBAGE_COLLECT_PERIODIC` (default) |
| Torpedo mesh reused across scenes | `GARBAGE_COLLECT_NEVER` |
| UI texture shared by all scenes | `GARBAGE_COLLECT_NEVER` (or use `shared_assets`) |
| Temporary procedural texture | `GARBAGE_COLLECT_PERIODIC` |

### The Torpedo Example

A classic scenario: a torpedo mesh is loaded once, reused every time the player fires, but there may be periods when no torpedoes are active:

```cpp
// During game initialization:
torpedo_mesh_id_ = assets->load_mesh(
    "torpedo.obj",
    MeshLoadOptions(),
    GARBAGE_COLLECT_NEVER  // Keep it forever
)->id();

// When the player fires:
auto torpedo_actor = create_child<Actor>();
auto mesh = assets->mesh(torpedo_mesh_id_);
torpedo_actor->set_mesh(mesh, DETAIL_LEVEL_NEAREST);

// When the torpedo detonates, the actor is destroyed
// but the mesh persists because of GARBAGE_COLLECT_NEVER

// When the game truly ends:
auto mesh = assets->mesh(torpedo_mesh_id_);
mesh->set_garbage_collection_method(GARBAGE_COLLECT_PERIODIC);
assets->destroy_mesh(torpedo_mesh_id_);
```

### Triggering Garbage Collection

The engine calls `run_garbage_collection()` automatically as part of its update cycle. You can also call it manually:

```cpp
assets->run_garbage_collection();
```

This is useful in tests or when you need to verify that an asset has been cleaned up:

```cpp
auto mesh = assets->create_mesh(spec);
AssetID id = mesh->id();
mesh.reset();

assert_true(assets->has_mesh(id));     // Still there, GC hasn't run yet
assets->run_garbage_collection();
assert_false(assets->has_mesh(id));    // Now it's gone
```

---

## 11. Best Practices for Managing Resource Lifetimes

### 1. Store IDs, Not Pointers

Always store IDs as class members. Resolve to pointers only when needed.

```cpp
class InventoryItem {
    AssetID icon_texture_id_;    // Good
    AssetID model_mesh_id_;      // Good
};

// NOT:
class InventoryItem {
    TexturePtr icon_texture_;    // Bad: prevents GC
    MeshPtr model_mesh_;         // Bad: prevents GC
};
```

### 2. Use Scope Blocks for Asset Access

```cpp
{
    auto mat = assets->material(mat_id);
    mat->set_diffuse_color(Color(1, 0, 0, 1));
    mat->set_shininess(0.5f);
}
```

### 3. Use Single-Line Access for One-Shot Calls

```cpp
assets->mesh(mesh_id)->recalculate_normals();
```

### 4. Check Existence Before Use

```cpp
if (assets->has_mesh(id)) {
    auto mesh = assets->mesh(id);
    // ...
}
```

### 5. Use the Right Asset Manager

- Scene-specific assets: `assets->load_X()`
- Shared/global assets: `window->shared_assets->load_X()`

### 6. Name Your Assets for Debugging

```cpp
auto mesh = assets->load_mesh("hero.glb");
mesh->set_name("HeroMesh");

// Later, find by name:
auto found = assets->find_mesh("HeroMesh");
```

### 7. Destroy Assets Explicitly When Done

```cpp
// When you are absolutely sure nothing references this asset:
assets->destroy_texture(tex_id);
assets->run_garbage_collection();
```

### 8. Attach Assets to StageNodes for Automatic Lifetime Management

When you attach a mesh to an Actor, the Actor holds a reference to it. When the Actor is destroyed, that reference is released:

```cpp
auto mesh = assets->load_mesh("tree.glb");
auto tree = create_child<Actor>(mesh);
mesh.reset();  // Release your reference, Actor still holds it

// When tree is destroyed, the mesh reference is released and it will be GC'd
```

### 9. Use `GARBAGE_COLLECT_NEVER` Sparingly

Only disable GC for assets that are genuinely long-lived and reused. Always re-enable it or explicitly destroy these assets when done.

### 10. Load Assets in `on_load()`, Start Using Them in `on_activate()`

```cpp
void GameScene::on_load() {
    // Load heavy assets
    hero_mesh_ = assets->load_mesh("hero.glb");
    level_mesh_ = assets->load_mesh("level_1.glb");
}

void GameScene::on_activate() {
    // Use the preloaded assets
    auto hero = create_child<Actor>(hero_mesh_);
    auto level = create_child<Actor>(level_mesh_);
}
```

---

## 12. Common Pitfalls and How to Avoid Them

### Pitfall 1: Forgetting to Store the Result of `load_X()`

```cpp
// BUG: The mesh is loaded and immediately destroyed
assets->load_mesh("hero.glb");  // Returns MeshPtr, but we ignore it
// GC runs, ref count is 1 (manager only), mesh is deleted

// FIX: Store the result
MeshPtr hero = assets->load_mesh("hero.glb");
```

### Pitfall 2: Holding Asset Pointers as Class Members

```cpp
// BUG:
class Player {
    MeshPtr mesh_;  // Prevents garbage collection forever
};

// FIX:
class Player {
    MeshID mesh_id_;

    MeshPtr get_mesh(AssetManager* assets) {
        return assets->mesh(mesh_id_);
    }
};
```

### Pitfall 3: Not Checking `has_X()` Before Access

```cpp
// BUG:
MeshPtr mesh = assets->mesh(some_id);  // Returns null if destroyed
mesh->recalculate_normals();           // CRASH

// FIX:
if (assets->has_mesh(some_id)) {
    MeshPtr mesh = assets->mesh(some_id);
    mesh->recalculate_normals();
}
```

### Pitfall 4: Mixing Scene and Shared Assets Confusingly

```cpp
// BUG: Loading a UI texture through scene assets
void MenuScene::on_load() {
    auto button_tex = assets->load_texture("ui/button.png");
}
// When MenuScene unloads, button_tex is released.
// If GameScene also tries to use it via the ID, it will be gone.

// FIX: Load shared UI assets through shared_assets
void MyGame::init() {
    auto button_tex = window->shared_assets->load_texture("ui/button.png");
}
```

### Pitfall 5: Deadlocks from Long-Held Pointers

```cpp
// BUG:
auto mesh = assets->mesh(mesh_id);
do_lengthy_operation();  // Another thread blocks trying to access mesh
mesh->draw();

// FIX:
{
    auto mesh = assets->mesh(mesh_id);
    mesh->draw();
}
do_lengthy_operation();
```

### Pitfall 6: Destroying Assets That Are Still Referenced

```cpp
// BUG:
auto mesh = assets->load_mesh("hero.glb");
actor->set_mesh(mesh, DETAIL_LEVEL_NEAREST);
assets->destroy_mesh(mesh->id());  // Actor still references this mesh!

// FIX: Let GC handle it, or ensure no references exist first
actor->set_mesh(MeshPtr(), DETAIL_LEVEL_NEAREST);  // Remove reference
assets->destroy_mesh(mesh->id());
```

### Pitfall 7: Assuming `destroy()` Is Immediate for StageNodes

```cpp
// BUG:
actor->destroy();
actor->move_to(1, 2, 3);  // Still works! Destruction is deferred

// FIX:
actor->destroy();
actor = nullptr;  // Clear your pointer so you don't accidentally use it
```

StageNodes are destroyed after `late_update()` but before the render queue is built. They remain accessible until then.

### Pitfall 8: Loading the Same Asset Multiple Times

```cpp
// INEFFICIENT:
void render_frame() {
    auto tex = assets->load_texture("hud.png");  // Loads every frame!
}

// FIX: Load once, store the ID
void GameScene::on_load() {
    hud_tex_id_ = assets->load_texture("hud.png")->id();
}

void GameScene::on_update(float dt) {
    if (assets->has_texture(hud_tex_id_)) {
        auto tex = assets->texture(hud_tex_id_);
        // Use it
    }
}
```

---

## Summary

| Concept | Rule |
|---------|------|
| IDs | Store IDs in your classes; they are cheap, safe, and serializable |
| Pointers | Borrow pointers from managers only when needed; release ASAP |
| Scope blocks | Wrap asset access in `{ }` to minimize lock duration |
| Existence checks | Always use `has_X(id)` before resolving an ID to a pointer |
| Ref counting | Assets are `shared_ptr`-based; holding a pointer prevents GC |
| StageNodes | Not ref-counted; owned by the Scene; use `destroy()` to remove |
| Scene assets | Tied to scene lifecycle; released when scene unloads |
| Shared assets | Global; persist across scenes; accessed via `window->shared_assets` |
| Garbage collection | `PERIODIC` (default) auto-cleans; `NEVER` keeps until you change it |
| Deadlocks | Caused by long-held pointers; solved by scope blocks and single-line access |
