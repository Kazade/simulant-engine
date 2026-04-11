# Performance Optimization Guide

This guide covers practical techniques for optimizing games built with the Simulant engine. It is aimed at junior developers who want concrete, actionable advice rather than abstract theory. Every section includes before/after code examples you can apply directly.

**Prerequisites:** Understand the basics of [Scene Graph](../core-concepts/stage-nodes.md), [Actors](../actors.md), [Geoms](../partitioners.md), [Assets](../assets/asset-managers.md), and [Physics](../physics/overview.md).

---

## Table of Contents

1. [Measuring Performance](#1-measuring-performance)
2. [Rendering Optimization](#2-rendering-optimization)
3. [Memory Management](#3-memory-management)
4. [Physics Optimization](#4-physics-optimization)
5. [Scene Graph Optimization](#5-scene-graph-optimization)
6. [Asset Loading](#6-asset-loading)
7. [Platform-Specific Optimization](#7-platform-specific-optimization)
8. [Code Optimization](#8-code-optimization)
9. [Common Performance Pitfalls](#9-common-performance-pitfalls)
10. [Optimization Checklist](#10-optimization-checklist)

---

## 1. Measuring Performance

You cannot optimize what you cannot measure. Always profile before you optimize.

### 1.1 Profiling with `SIMULANT_PROFILE`

Simulant has a built-in profiler that is safe to leave in production code -- it compiles to nothing when profiling is disabled.

**Enable profiling one of three ways:**

```bash
# Method 1: Environment variable (easiest for quick testing)
SIMULANT_PROFILE=1 ./my_game

# Method 2: Set in code (for specific builds)
config.development.force_profiling = true;

# Method 3: Compile-time flag (permanent)
cmake -DSIMULANT_PROFILE=ON ..
```

**What profiling mode does:**
- Disables vsync
- Disables frame limiting
- Enables internal timing (Dreamcast also writes `gmon.out` for `gprof`)

**Using the profiler macros:**

Mark sections of your code with `S_PROFILE_SECTION` to measure their execution time:

```cpp
void update_level() {
    S_PROFILE_SECTION("Level Update");

    update_enemies();

    S_PROFILE_SECTION("Physics");
    physics_scene_->step_simulation(1.0f / 60.0f);

    S_PROFILE_SUBSECTION("Collision checks");
    check_special_collisions();

    S_PROFILE_SECTION("Rendering Prep");
    update_particle_systems();
}

// At shutdown (or periodically):
S_PROFILE_DUMP_TO_STDOUT();
```

**Output looks like:**

```
Level Update                      16500us
Physics                           8200us
Physics / Collision checks        3100us
Rendering Prep                    4800us
```

The macros are no-ops when `SIMULANT_PROFILE` is not defined, so there is zero runtime cost in release builds. See [profiler.h](../../simulant/tools/profiler.h) for the implementation.

### 1.2 StatsPanel (FPS Display)

The `StatsPanel` stage node displays real-time performance statistics on screen:

```cpp
class GameScene : public Scene<GameScene> {
private:
    StatsPanel* stats_panel_ = nullptr;

    void on_load() override {
        // ... your game setup ...

        // Add the stats panel as a child of the scene
        stats_panel_ = create_child<StatsPanel>();
    }
};
```

**The panel shows:**
- FPS (frames per second)
- Frame time (ms per frame)
- RAM usage
- VRAM usage
- Actors rendered count
- Polygons rendered count

Use `StatsPanel` during development to spot regressions immediately. See [StatsPanel](../../simulant/nodes/stats_panel.h).

### 1.3 Frame Timing

Simulant provides frame timing through the `TimeKeeper`. You can measure frame duration manually:

```cpp
void on_update(float dt) override {
    uint64_t frame_start = application->time_keeper->now_in_us();

    // ... your update code ...

    uint64_t frame_end = application->time_keeper->now_in_us();
    S_INFO("Frame took {} microseconds", frame_end - frame_start);
}
```

### 1.4 Identifying Bottlenecks

Use a systematic approach to identify bottlenecks:

1. **Enable profiling mode** and run your game under worst-case conditions (most enemies, most particles, etc.)
2. **Check StatsPanel** for baseline FPS and draw call counts
3. **Add `S_PROFILE_SECTION`** to major subsystems in your update loop
4. **Compare timings** to identify the slowest section
5. **Narrow down** by adding subsections within the slowest area

**Typical bottleneck categories:**

| Symptom | Likely Cause | Where to Look |
|---------|-------------|---------------|
| Low FPS, low CPU | GPU-bound (too many polygons, large textures) | Rendering section below |
| Low FPS, high CPU | CPU-bound (too many updates, physics) | Physics / Scene Graph sections |
| Stuttering | Asset loading during gameplay | Asset Loading section |
| High memory usage | Leaked asset references | Memory Management section |

---

## 2. Rendering Optimization

Rendering is the most common performance bottleneck. Simulant provides several mechanisms to reduce rendering cost.

### 2.1 Draw Call Batching

Simulant automatically batches draw calls that share the same material and render state. The engine groups renderables by a `RenderGroup` key (pass, transparency, texture, material) and processes them in order.

**How to help the batcher:**

**Use shared materials** for objects that render together:

```cpp
// BAD: Each enemy gets its own material = more draw calls
for (int i = 0; i < 50; ++i) {
    auto enemy = create_child<Actor>(enemy_mesh_id);
    auto mat = assets->new_material();
    mat->set_diffuse_map(enemy_tex);
    enemy->set_material(mat->id());
}

// GOOD: All enemies share one material = fewer draw calls
auto shared_mat = assets->new_material();
shared_mat->set_diffuse_map(enemy_tex);
shared_mat->set_garbage_collection_method(GARBAGE_COLLECT_NEVER);

for (int i = 0; i < 50; ++i) {
    auto enemy = create_child<Actor>(enemy_mesh_id);
    enemy->set_material(shared_mat->id());
}
```

**Minimize transparent objects.** Transparent objects sort back-to-front and cannot batch as efficiently:

```cpp
// Keep transparent passes minimal
material_pass->set_blending_enabled(true);  // Only when necessary
```

### 2.2 Geom vs Actor (Static vs Dynamic)

Understanding when to use `Geom` versus `Actor` is critical for rendering performance.

| Feature | `Actor` | `Geom` |
|---------|---------|--------|
| Movable | Yes, every frame | No, fixed at creation |
| Internal culling | None | Octree/Quadtree |
| Best for | Characters, vehicles, moving objects | Levels, buildings, terrain |
| Overhead | Low per frame | Higher setup, near-zero per frame |

**Use `Actor` for moving objects:**

```cpp
auto player = create_child<Actor>(player_mesh_id, DETAIL_LEVEL_NEAREST);
player->move_by(dx, dy, dz);  // Updated every frame
```

**Use `Geom` for static geometry:**

```cpp
auto level_geom = create_child<Geom>(level_mesh_id, position, orientation);
// Position/orientation set once at creation and never change
// Internally uses Octree culling for polygon-level visibility
```

**Before/after: Converting static Actors to Geoms**

```cpp
// BEFORE: Using Actors for level geometry (slow)
void load_level() {
    auto wall1 = create_child<Actor>(wall_mesh_id);
    wall1->move_to(10, 0, 0);

    auto floor = create_child<Actor>(floor_mesh_id);
    floor->move_to(0, -1, 0);

    auto ceiling = create_child<Actor>(ceiling_mesh_id);
    ceiling->move_to(0, 10, 0);
    // These are processed every frame even though they never move
}

// AFTER: Using Geoms for static geometry (fast)
void load_level() {
    auto wall1 = create_child<Geom>(wall_mesh_id, Vec3(10, 0, 0));

    auto floor = create_child<Geom>(floor_mesh_id, Vec3(0, -1, 0));

    auto ceiling = create_child<Geom>(ceiling_mesh_id, Vec3(0, 10, 0));
    // These use Octree culling -- only visible polygons are submitted
}
```

**Warning:** Once a `Geom` is created, do NOT manipulate the underlying mesh's `vertex_data`. Changing vertex positions or counts will cause visual corruption or crashes. You may be able to safely modify diffuse colours, texture coordinates, or normals.

### 2.3 Partitioners and Culling

Simulant uses **partitioners** to quickly determine which scene nodes are visible from a camera. The partitioner is selected when a `Stage` is constructed via `new_stage()`.

Partitioners provide two methods:
- `geometry_visible_from(CameraID)` -- returns visible geometry
- `lights_visible_from(CameraID)` -- returns visible lights

**Available partitioners:**

| Partitioner | Best For | Description |
|-------------|----------|-------------|
| `PartitionerFrustum` | General 3D scenes | Frustum-based culling |
| `PartitionerSpatialHash` | Large open worlds | Spatial hash grid (see [Spatial Hashing](../spatial_hashing.md)) |

**How it works:** The partitioner tracks all `StageNodes` and culls off-screen ones before they reach the renderer. If a `Geom` passes the partitioner check, a second Octree culling step runs to cull individual polygon groups within the Geom.

### 2.4 Octree/Quadtree Culling

`Geom` nodes internally use an **Octree** (3D) or **Quadtree** (2D/terrain) to subdivide their mesh into regions. Only regions inside the camera frustum are rendered.

**Configuring culling when creating a Geom:**

```cpp
GeomCullerOptions opts;
opts.type = GEOM_CULLER_TYPE_OCTREE;
opts.octree_max_depth = 5;         // Deeper = finer culling, more memory
opts.octree_min_primitives = 10;   // Stop subdividing below this count

auto level = create_child<Geom>(level_mesh_id, position, orientation, opts);
```

**Choosing Octree vs Quadtree:**

- **Octree**: Best for enclosed 3D levels (interiors, dungeons, buildings)
- **Quadtree**: Best for terrain and flat outdoor scenes

### 2.5 LOD (Level of Detail)

Simulant supports up to 5 detail levels per `Actor`. The engine automatically selects the appropriate level based on distance to the camera.

**Setting up LOD:**

```cpp
auto hero = create_child<Actor>();

// Attach different meshes at different detail levels
hero->set_mesh(hero_high_poly_id, DETAIL_LEVEL_NEAREST);   // Close up
hero->set_mesh(hero_med_poly_id, DETAIL_LEVEL_NEAR);       // Medium distance
hero->set_mesh(hero_low_poly_id, DETAIL_LEVEL_MID);        // Far away
hero->set_mesh(hero_vlow_poly_id, DETAIL_LEVEL_FAR);       // Very far
hero->set_mesh(hero_lowest_id, DETAIL_LEVEL_FARTHEST);     // Barely visible

// Configure distance cutoffs on the pipeline
pipeline->set_detail_level_distances(
    10.0f,   // Anything closer than 10 units uses NEAREST
    20.0f,   // 10-20 uses NEAR
    40.0f,   // 20-40 uses MID
    80.0f    // 40-80 uses FAR, 80+ uses FARTHEST
);
```

**Query the active detail level:**

```cpp
auto level = pipeline->detail_level_at_distance(25.0f);
// Returns DETAIL_LEVEL_MID with the cutoffs above
```

**Practical tip:** For distant objects, a simple billboard or even a `DETAIL_LEVEL_FARTHEST` mesh with just 2 triangles is often sufficient.

### 2.6 Material Sharing

Every unique material is a potential separate draw call. Sharing materials across objects enables the renderer to batch them.

```cpp
// Create one material for all crates
auto crate_material = assets->new_material();
crate_material->set_diffuse_map(crate_texture_id);
crate_material->set_garbage_collection_method(GARBAGE_COLLECT_NEVER);

// All crate actors share this material
for (int i = 0; i < 100; ++i) {
    auto crate = create_child<Actor>(crate_mesh_id);
    crate->set_material(crate_material->id());
}
```

**Material passes:** A material can have multiple passes. Each pass is a separate draw call. Minimize the number of passes:

```cpp
// BAD: Two passes when one will do
auto pass0 = material->passes()->add();
pass0->set_diffuse_map(tex1);
auto pass1 = material->passes()->add();
pass1->set_diffuse_map(tex2);

// GOOD: Combine into a single texture if possible
material->set_diffuse_map(combined_tex);
```

### 2.7 Texture Optimization

**Texture size limits by platform:**

| Platform | Max Texture Size | Notes |
|----------|-----------------|-------|
| Desktop | GPU-dependent | Typically 8192+ |
| PSP | **512 pixels** | Must be power-of-two |
| Dreamcast (GL1X) | **1024 pixels** | Power-of-two recommended |

**Tips:**

- Use **power-of-two** textures on all platforms (required on PSP)
- Use **DDS** files with DXT compression on desktop (reduces VRAM)
- Use **DTEX/KMG** formats on Dreamcast (native KallistiOS formats)
- Use **texture atlases** to reduce texture swaps (see [Sprites](../rendering/sprites.md))
- Avoid per-frame texture swaps -- preload all needed textures

```cpp
// BAD: Loading a new texture every frame
void update() {
    material->set_diffuse_map(assets->load_texture("frame_" + std::to_string(current_frame) + ".png"));
}

// GOOD: Preload all frames once, swap IDs
std::vector<TextureID> frames;
void on_load() {
    for (int i = 0; i < frame_count; ++i) {
        frames.push_back(assets->load_texture("frame_" + std::to_string(i) + ".png")->id());
    }
}

void update() {
    material->set_diffuse_map(frames[current_frame]);
}
```

---

## 3. Memory Management

Simulant runs on platforms with as little as **16MB RAM** (Dreamcast). Proper memory management is essential.

### 3.1 Asset Lifecycle and Garbage Collection

All assets are reference-counted via `std::shared_ptr`. When the reference count drops to **1** (only the manager's internal reference remains), the asset is eligible for garbage collection.

**Garbage collection methods:**

| Method | Behaviour | Use Case |
|--------|-----------|----------|
| `GARBAGE_COLLECT_PERIODIC` (default) | Freed when ref count reaches 1 and GC runs | Level-specific assets |
| `GARBAGE_COLLECT_NEVER` | Persists until you change the method | Shared/reusable assets |

```cpp
// Default: auto-cleaned when no longer referenced
auto level_mesh = assets->load_mesh("level.obj");

// Persistent: survives GC runs
auto ui_texture = assets->load_texture("ui.png", GARBAGE_COLLECT_NEVER);

// Switch back to periodic when done
ui_texture->set_garbage_collection_method(GARBAGE_COLLECT_PERIODIC);

// Trigger cleanup
assets->run_garbage_collection();
```

GC runs automatically during the engine's update cycle. You can also trigger it manually.

### 3.2 Reference Counting

Every `shared_ptr` copy increments the reference count. Internal asset references also count -- a `Mesh` references its `Material`, which references `Texture`s. This prevents premature cleanup:

```cpp
auto tex = assets->load_texture("hero.png");
auto mat = assets->new_material();
mat->set_diffuse_map(tex);  // tex's ref count increases

tex.reset();                // tex is NOT freed -- material still references it
mat.reset();                // Now tex is eligible for GC
```

**Debugging:** Use `ptr.use_count()` to check reference counts during development. A count of `1` means only the manager holds the reference.

### 3.3 AssetID vs Pointer Storage

**This is the single most important rule in Simulant:**

> **Store `AssetID` (e.g. `MeshID`, `TextureID`), not `shared_ptr` (e.g. `MeshPtr`).**
> Holding a `shared_ptr` as a class member prevents garbage collection and causes memory leaks.

```cpp
// BAD: Holding a shared_ptr prevents garbage collection
class Player {
    MeshPtr mesh_;  // Reference count never drops to 1!
};

// GOOD: Store the ID, borrow the pointer when needed
class Player {
    MeshID mesh_id_;  // No ownership, no GC interference

    void update() {
        // Scope block: pointer is released at closing brace
        {
            auto mesh = assets->mesh(mesh_id_);
            // Use mesh...
        }  // mesh shared_ptr destroyed here
    }
};
```

See [Resource Management](../core-concepts/resource-management.md) for full details.

### 3.4 Asset Unloading

When switching scenes, scene-specific assets should be released:

```cpp
void on_deactivate() override {
    // Remove references to scene-specific assets
    level_mesh_id_ = AssetID();
    level_music_id_ = AssetID();

    // Force garbage collection to free scene assets
    assets->run_garbage_collection();
}
```

**Keep shared assets in `shared_assets`:** The `SceneManager` provides a `shared_assets` manager that persists across scene transitions:

```cpp
// Load UI assets into shared_assets (not scene-specific assets)
auto shared = scenes->shared_assets();
auto ui_font = shared->load_font("font.ttf", GARBAGE_COLLECT_NEVER);
```

### 3.5 Memory Pools

Simulant does not yet have a built-in memory pool system (it is on the [todo list](../todo.md)). For frequently created/destroyed objects (projectiles, particles), use this pattern:

```cpp
class ProjectilePool {
public:
    void initialize(size_t count, MeshID mesh_id) {
        mesh_id_ = mesh_id;
        for (size_t i = 0; i < count; ++i) {
            auto proj = scene_->create_child<Actor>(mesh_id_, DETAIL_LEVEL_NEAREST);
            proj->set_visible(false);
            proj->set_enabled(false);
            pool_.push_back(proj);
        }
    }

    Actor* acquire() {
        for (auto& proj : pool_) {
            if (!proj->is_enabled()) {
                proj->set_visible(true);
                proj->set_enabled(true);
                return proj.get();
            }
        }
        return nullptr;  // Pool exhausted
    }

    void release(Actor* proj) {
        proj->set_visible(false);
        proj->set_enabled(false);
    }

private:
    MeshID mesh_id_;
    std::vector<ActorPtr> pool_;
};
```

This avoids repeated `create_child`/`destroy` calls and the associated memory allocations.

---

## 4. Physics Optimization

Simulant's physics system is built on the **Bounce** physics library. Physics simulation is computationally expensive -- optimize it carefully.

### 4.1 Use Simple Collider Shapes

Collider complexity directly impacts performance. Prefer simpler shapes:

```cpp
// FAST: Sphere (single radius check)
body->add_sphere_collider(radius, PhysicsMaterial());

// FAST: Box (AABB overlap test)
body->add_box_collider(size, PhysicsMaterial());

// MEDIUM: Capsule (sphere + cylinder)
body->add_capsule_collider(bottom, top, diameter, PhysicsMaterial());

// SLOW: Triangle mesh (per-triangle collision)
body->add_triangle_collider(mesh, PhysicsMaterial());  // Avoid for moving objects!
```

**Rules of thumb:**
- Use **spheres** for balls, pickups, simple triggers
- Use **boxes** for crates, platforms, walls
- Use **capsules** for characters (good balance of accuracy and speed)
- Use **triangle mesh** only for static terrain, never for dynamic objects

### 4.2 Disable Sleeping Bodies

Physics bodies that have come to rest can be put to "sleep" to skip simulation. However, if bodies are constantly woken up (e.g. by nearby explosions), the sleep/wake cycle itself costs CPU.

```cpp
// For a scene with many small debris that should settle:
body->set_sleeping_enabled(true);   // Allow bodies to sleep when still
body->set_sleep_threshold(0.1f);    // Lower = sleeps sooner

// For bodies that are constantly active (e.g. a spinning fan):
body->set_sleeping_enabled(false);  // Don't waste time checking sleep state
```

**To disable all sleeping** (useful for debugging):
```cpp
physics_scene_->set_sleeping_enabled(false);
```

### 4.3 Collision Layers

Use collision layers to skip unnecessary collision checks:

```cpp
class CustomContactFilter : public ContactFilter {
public:
    bool should_collide(FixturePtr a, FixturePtr b) override {
        // Skip collision between two sensor fixtures
        if (a->is_sensor() && b->is_sensor()) {
            return false;
        }

        // Skip collision between pickups and each other
        if (a->get_group() == GROUP_PICKUP && b->get_group() == GROUP_PICKUP) {
            return false;
        }

        return true;  // Default: collide
    }
};

auto physics_service = application->find_service<PhysicsService>();
physics_service->set_contact_filter(&custom_filter);
```

### 4.4 Reducing Simulation Frequency

The physics simulation runs at a fixed timestep. Reducing the frequency saves CPU at the cost of accuracy:

```cpp
// Default: 60 Hz simulation
physics_scene_->set_fixed_timestep(1.0f / 60.0f);

// Reduce to 30 Hz for less critical physics
physics_scene_->set_fixed_timestep(1.0f / 30.0f);

// Pause physics entirely when not needed
physics_scene_->set_enabled(false);  // Paused
// ... game is paused, showing menu ...
physics_scene_->set_enabled(true);   // Resumed
```

**When to reduce frequency:**
- Background levels the player cannot see
- Simple scenes with few dynamic objects
- On constrained platforms (Dreamcast, PSP) where CPU is limited

---

## 5. Scene Graph Optimization

The scene graph is the hierarchy of `StageNode` objects. A deep or busy scene graph adds overhead to every frame.

### 5.1 Keep Hierarchies Shallow

Every node in the hierarchy adds transform calculation overhead during updates.

```cpp
// BAD: Deep hierarchy (5 levels of nesting)
auto root = create_child<Actor>();
  auto body = root->create_child<Actor>();
    auto weapon = body->create_child<Actor>();
      auto muzzle = weapon->create_child<Actor>();
        auto flash = muzzle->create_child<Actor>();

// GOOD: Flatter hierarchy (3 levels)
auto body = create_child<Actor>();
  auto weapon = create_child<Actor>();
  auto muzzle_flash = create_child<Actor>();  // Positioned relative to weapon manually
```

### 5.2 Minimize Node Count

Each `StageNode` receives update calls, participates in partitioner culling, and contributes to render queue generation.

**Remove nodes you do not need:**

```cpp
// BAD: Invisible node just to hold a reference
auto marker = create_child<Actor>();
marker->set_visible(false);
marker->move_to(spawn_point);

// GOOD: Store the position directly
Vec3 spawn_point = {10.0f, 0.0f, 5.0f};
```

### 5.3 Update Throttling

Nodes that are part of an active pipeline receive `on_update()` and `on_fixed_update()` calls every frame. If a node does not need per-frame updates, consider disabling it:

```cpp
// Disable updates for nodes that only need to render
decorative_node->set_update_enabled(false);

// Re-enable when you need updates again
decorative_node->set_update_enabled(true);
```

Alternatively, use signals or coroutines instead of per-frame polling:

```cpp
// BAD: Polling every frame to check a condition
void on_update(float dt) override {
    if (player->position().distance_to(trigger_point) < 2.0f) {
        activate_trap();
    }
}

// GOOD: Check less frequently using a coroutine
cr_async([this]() {
    while (true) {
        if (player->position().distance_to(trigger_point) < 2.0f) {
            activate_trap();
            break;
        }
        cr_yield_for(Seconds(0.5f));  // Check twice per second instead of 60 times
    }
});
```

### 5.4 Disabling Off-Screen Nodes

Nodes are culled by the partitioner by default. You can also manually disable nodes that you know are off-screen:

```cpp
// Toggle visibility for nodes outside the camera view
if (!is_in_view(node)) {
    node->set_visible(false);
    node->set_enabled(false);  // Stops update calls too
} else {
    node->set_visible(true);
    node->set_enabled(true);
}
```

For nodes that should never be culled (e.g. a persistent UI overlay), disable culling:

```cpp
node->set_cullable(false);  // Always rendered regardless of position
```

---

## 6. Asset Loading

Loading assets during gameplay causes frame hitches. Use these techniques to load without interrupting the player.

### 6.1 Background Preloading

The `SceneManager` can load scenes asynchronously in the background:

```cpp
class MenuScene : public Scene<MenuScene> {
    void on_load() override {
        // Start preloading the game scene in the background
        scenes->preload_in_background("game", level_number).then([this]() {
            // This coroutine callback runs when preloading is complete
            S_INFO("Game scene is loaded and ready");
            game_is_ready_ = true;
        });
    }

    void on_update(float dt) override {
        // Player presses "Start"
        if (start_pressed && game_is_ready_) {
            scenes->switch_to("game");  // Instant -- already loaded
        }
    }

private:
    bool game_is_ready_ = false;
};
```

See [Scene Management](../core-concepts/scene-management.md) for full details.

### 6.2 Async Loading with Coroutines

Use coroutines for custom loading sequences:

```cpp
void load_level_async(int level_id) {
    cr_async([this, level_id]() {
        // Show loading screen
        loading_screen->set_visible(true);

        // Load assets one at a time, yielding between each
        auto mesh = assets->load_mesh("levels/level_" + std::to_string(level_id) + ".obj");
        cr_yield();  // Let the frame render

        auto tex = assets->load_texture("levels/level_" + std::to_string(level_id) + ".png");
        cr_yield();

        auto music = assets->load_sound("music/level_" + std::to_string(level_id) + ".ogg");
        cr_yield();

        // All loaded -- hide loading screen
        loading_screen->set_visible(false);
    });
}
```

The `cr_yield()` call lets the engine render a frame before continuing. Use `cr_yield_for(Seconds(n))` to wait for a specific duration.

### 6.3 Asset Preloading

For assets you know you will need soon, preload them synchronously during loading screens:

```cpp
void on_load() override {
    // Preload all assets for this level synchronously
    // (This blocks, but we're in a loading screen so that is fine)
    hero_mesh_ = assets->load_mesh("hero.obj", GARBAGE_COLLECT_NEVER);
    hero_tex_ = assets->load_texture("hero.png", GARBAGE_COLLECT_NEVER);
    level_mesh_ = assets->load_mesh("level.obj");

    // Now gameplay is smooth
}
```

### 6.4 Streaming Pattern

For very large levels, stream assets in chunks:

```cpp
void stream_level(int level_id) {
    cr_async([this, level_id]() {
        // Load the immediate area first
        load_zone(0, 0);  // Center zone

        player_spawned_ = true;

        // Load surrounding zones in the background
        for (int x = -1; x <= 1; ++x) {
            for (int z = -1; z <= 1; ++z) {
                if (x == 0 && z == 0) continue;  // Already loaded
                load_zone(x, z);
                cr_yield();  // Yield between zones
            }
        }
    });
}
```

**Tip:** On platforms with optical media (Dreamcast CD), file reads can interrupt CD audio playback. Use `vfs->enable_read_blocking(true)` during music playback to prevent audio glitches.

---

## 7. Platform-Specific Optimization

Simulant targets platforms with vastly different capabilities. Write once, but optimize per-platform.

### 7.1 Dreamcast

The Sega Dreamcast is the most constrained target:

| Spec | Value |
|------|-------|
| CPU | 200MHz SH-4 |
| RAM | 16MB |
| GPU | PowerVR2 (GL1X renderer) |
| Max lights per object | 2 |
| Max texture size | 1024px |
| Audio | OGG Vorbis (streaming) |

**Key optimizations:**

```cpp
// 1. Reduce polygon counts drastically
// Use LOD aggressively -- even medium distance should use very low-poly meshes

// 2. Use native texture formats
// Convert textures to DTEX or KMG format for faster loading and lower VRAM

// 3. Minimize light count
// Max 2 lights per object. Use baked lighting where possible.

// 4. Embed assets
// Assets are built into the executable. Ensure simulant.json lists all asset paths.
// Build with: simulant build dreamcast

// 5. Avoid skyboxes
// Cube map textures may exceed memory limits. Use simple gradient backgrounds instead.

// 6. Conditional compilation
#ifdef SIMULANT_PLATFORM_DREAMCAST
    auto mesh = assets->load_mesh("meshes/hero_dc.obj");  // Low-poly variant
#else
    auto mesh = assets->load_mesh("meshes/hero.obj");
#endif

// 7. Use the built-in profiler
// Run via dcload with -c flag, generates gmon.out for gprof analysis
// sh-elf-gprof -b -p my_game.elf.debug gmon.out
```

### 7.2 PSP

The PSP is moderately constrained:

| Spec | Value |
|------|-------|
| CPU | 333MHz MIPS |
| RAM | 32MB |
| GPU | Custom PSP renderer |
| Max lights per object | 4 |
| Max texture size | **512px (power-of-two required)** |
| Audio | WAV only (no OGG) |

**Key optimizations:**

```cpp
// 1. All textures MUST be power-of-two
// 256x256, 512x512, 1024x512 -- non-power-of-two will fail

// 2. Use WAV for audio (OGG not supported)
#ifdef SIMULANT_PLATFORM_PSP
    auto music = assets->load_sound("music/level.wav");
#else
    auto music = assets->load_sound("music/level.ogg");
#endif

// 3. Reduce texture resolution
// 512px max -- use 256x256 where possible to save VRAM

// 4. Lower polygon counts
// Similar to Dreamcast but slightly more headroom

#ifdef SIMULANT_PLATFORM_PSP
    // PSP-specific code
#endif
```

### 7.3 Desktop (Linux, Windows)

Desktop has far more resources but different optimization opportunities:

**Key optimizations:**

- **Use DDS textures** with DXT compression to reduce VRAM usage and load times
- **Multi-core:** Simulant's asset loading uses background threads -- take advantage by preloading in parallel
- **Higher detail levels:** Use high-poly meshes, more lights (up to 8 per object), larger textures
- **GL2X renderer:** Supports custom shaders, more features than GL1X

```cpp
// Platform-conditional asset loading
auto get_platform_name() -> std::string {
    return get_platform()->name();
}

void load_assets() {
    if (get_platform_name() != "dreamcast" && get_platform_name() != "psp") {
        // Desktop: use high-quality assets
        skybox = create_child<Actor>(skybox_mesh_id);
        high_quality_shadows = true;
    } else {
        // Constrained platforms: skip expensive features
        high_quality_shadows = false;
    }
}
```

---

## 8. Code Optimization

Small code-level improvements compound across thousands of per-frame calls.

### 8.1 Avoiding Allocations in Loops

Every allocation inside a loop creates garbage for the GC to clean up later:

```cpp
// BAD: Allocating a shared_ptr every frame
void on_update(float dt) override {
    for (auto& enemy : enemies_) {
        auto mesh = assets->mesh(enemy.mesh_id);  // Allocates shared_ptr
        mesh->recalculate_aabb();
    }
}

// GOOD: Borrow the pointer only when needed
void on_update(float dt) override {
    for (auto& enemy : enemies_) {
        if (assets->has_mesh(enemy.mesh_id)) {
            auto mesh = assets->mesh(enemy.mesh_id);
            mesh->recalculate_aabb();
        }  // shared_ptr released here
    }
}

// BEST: Single-line call for simple operations
void on_update(float dt) override {
    for (auto& enemy : enemies_) {
        // Temporary shared_ptr is destroyed immediately
        if (assets->has_mesh(enemy.mesh_id)) {
            assets->mesh(enemy.mesh_id)->recalculate_aabb();
        }
    }
}
```

### 8.2 Caching Lookups

Cache expensive lookups instead of repeating them:

```cpp
// BAD: Looking up the same asset repeatedly
void render_all_enemies() {
    for (auto& enemy : enemies_) {
        auto mat = assets->material(enemy.material_id);  // Lookup every iteration
        mat->set_uniform("health", enemy.health);
    }
}

// GOOD: Cache the lookup
void render_all_enemies() {
    {
        auto shared_mat = assets->material(shared_material_id_);  // One lookup
        for (auto& enemy : enemies_) {
            shared_mat->set_uniform("health", enemy.health);
        }
    }  // shared_mat released
}

// BAD: Repeated node lookups
void on_update(float dt) override {
    auto player = find<Actor>("Player");  // Searches scene graph every frame
    move_toward(player->position());
}

// GOOD: Cache the node reference (StageNodes are not reference-counted)
void on_load() override {
    player_ = find<Actor>("Player");  // One-time lookup
}

void on_update(float dt) override {
    if (player_) {
        move_toward(player_->position());
    }
}
```

### 8.3 Scope Blocks for Asset Access

The **scope block pattern** is the most important idiom in Simulant resource management. It ensures asset manager locks are held for the minimum possible time:

```cpp
// BAD: Holding a pointer across unrelated work
auto mesh = assets->mesh(mesh_id_);

do_expensive_calculation();   // mesh pointer still held
update_physics();             // mesh pointer still held
process_input();              // mesh pointer still held

mesh->recalculate_normals();  // Finally use it

// GOOD: Scope block releases the pointer immediately
{
    auto mesh = assets->mesh(mesh_id_);
    mesh->recalculate_normals();
}  // mesh released here -- lock freed

do_expensive_calculation();   // No lock held
update_physics();
process_input();
```

### 8.4 Signal Performance

Simulant's signals use a **linked list** of callbacks:

| Operation | Complexity | Notes |
|-----------|-----------|-------|
| `connect()` | O(1) | Appended to tail |
| `emit()` | O(n) | Sequential iteration |
| `disconnect()` | O(n) | List search |

**Guidelines:**

```cpp
// BAD: Connecting inside a per-frame loop
void on_update(float dt) override {
    for (auto& entity : entities_) {
        entity.signal_hit().connect([this]() {  // New connection every frame!
            on_entity_hit();
        });
    }
}

// GOOD: Connect once during initialization
void on_load() override {
    for (auto& entity : entities_) {
        entity.signal_hit().connect([this]() {
            on_entity_hit();
        });
    }
}

// BAD: 50 listeners on a per-frame signal = 50 calls per frame
// Consider batching:
struct BatchedEvent {
    std::vector<Entity*> hit_entities;
};
BatchedEvent batch;
for (auto& e : entities_) {
    if (e.was_hit()) batch.hit_entities.push_back(&e);
}
if (!batch.hit_entities.empty()) {
    signal_batched_hit().emit(batch);  // One signal, many entities
}
```

**Use `ScopedConnection` for automatic cleanup:**

```cpp
// Auto-disconnects when the connection object is destroyed
sig::scoped_connection conn = body->signal_contact_begin().connect(callback);
```

---

## 9. Common Performance Pitfalls

### Pitfall 1: Holding `shared_ptr` as a Class Member

```cpp
// BUG: This mesh will NEVER be garbage collected
class MyScene : public Scene<MyScene> {
    MeshPtr persistent_mesh_;  // Ref count stuck at 2+
};

// FIX: Store the ID instead
class MyScene : public Scene<MyScene> {
    MeshID persistent_mesh_id_;
};
```

### Pitfall 2: Loading Assets During Gameplay

```cpp
// BUG: Causes frame hitch every time a new enemy spawns
void spawn_enemy() {
    auto mesh = assets->load_mesh("enemy.obj");  // File I/O on the main thread!
    auto enemy = create_child<Actor>(mesh->id());
}

// FIX: Preload during level load
void on_load() override {
    enemy_mesh_id_ = assets->load_mesh("enemy.obj", GARBAGE_COLLECT_NEVER)->id();
}

void spawn_enemy() {
    auto enemy = create_child<Actor>(enemy_mesh_id_);  // Instant
}
```

### Pitfall 3: Triangle Mesh Colliders on Dynamic Objects

```cpp
// BUG: Per-triangle collision on a moving object is very expensive
body->add_triangle_collider(complex_mesh, PhysicsMaterial());

// FIX: Use a simple shape for the dynamic body
body->add_box_collider(Vec3(1, 1, 1), PhysicsMaterial());
body->add_sphere_collider(1.0f, PhysicsMaterial());
```

### Pitfall 4: Deep Scene Graph Hierarchies

```cpp
// BUG: 6 levels of nesting, each computing transforms every frame
root -> group1 -> group2 -> body -> weapon -> muzzle -> flash

// FIX: Flatten the hierarchy where possible
root -> body
    -> weapon (positioned manually)
    -> muzzle_flash (positioned manually)
```

### Pitfall 5: Creating/Destroying Connections Every Frame

```cpp
// BUG: Signal connect/disconnect every frame
void on_update(float dt) override {
    auto conn = player->signal_moved().connect(callback);
    // ...
    conn.disconnect();  // O(n) search every frame
}

// FIX: Connect once, disconnect when done
void on_load() override {
    move_conn_ = player->signal_moved().connect(callback);
}

void on_deactivate() override {
    move_conn_.disconnect();
}
```

### Pitfall 6: Not Unloading Scene Assets

```cpp
// BUG: Previous level's assets stay in memory
void switch_level(int new_level) {
    scenes->activate("level_" + std::to_string(new_level));
    // Old level's assets still referenced by shared_ptrs in destroyed scene
}

// FIX: Clear references and trigger GC
void on_deactivate() override {
    level_mesh_id_ = AssetID();
    level_music_id_ = AssetID();
    assets->run_garbage_collection();
}
```

---

## 10. Optimization Checklist

Use this checklist when preparing your game for release. Work through items in order -- each section builds on the previous one.

### Phase 1: Measure

- [ ] Enable `SIMULANT_PROFILE` and run worst-case scenarios
- [ ] Add `StatsPanel` to monitor FPS, draw calls, and memory
- [ ] Add `S_PROFILE_SECTION()` to major subsystems
- [ ] Identify the top 3 bottlenecks (CPU, GPU, memory, I/O)

### Phase 2: Rendering

- [ ] Replace static `Actor`s with `Geom`s for level geometry
- [ ] Configure Octree/Quadtree culling on Geoms
- [ ] Share materials across objects to improve batching
- [ ] Set up LOD levels for distant objects
- [ ] Reduce transparent object count
- [ ] Minimize material passes (1 pass per material where possible)
- [ ] Use texture atlases instead of many small textures
- [ ] Verify texture sizes are within platform limits

### Phase 3: Memory

- [ ] Replace all `MeshPtr`/`TexturePtr` members with `MeshID`/`TextureID`
- [ ] Use scope blocks `{ }` around all asset access
- [ ] Set `GARBAGE_COLLECT_NEVER` on reusable assets (projectiles, UI)
- [ ] Use `shared_assets` for cross-scene assets
- [ ] Clear asset references and call `run_garbage_collection()` on scene switch
- [ ] Check for leaked references with `ptr.use_count()`

### Phase 4: Physics

- [ ] Replace triangle mesh colliders with spheres/boxes where possible
- [ ] Enable sleeping on bodies that settle
- [ ] Disable sleeping on bodies that are constantly active
- [ ] Add collision filters to skip unnecessary checks
- [ ] Reduce simulation frequency for non-critical scenes
- [ ] Pause physics when the game is paused

### Phase 5: Scene Graph

- [ ] Flatten deep hierarchies (aim for 3 levels or fewer)
- [ ] Remove invisible marker nodes -- store positions directly
- [ ] Disable updates on nodes that only need to render
- [ ] Disable off-screen nodes manually if the partitioner misses them
- [ ] Use coroutines instead of per-frame polling where possible

### Phase 6: Loading

- [ ] Use `preload_in_background()` for next-level preloading
- [ ] Show a loading screen during asset loads
- [ ] Use `cr_yield()` between large asset loads to maintain responsiveness
- [ ] Preload all gameplay-critical assets during loading screens
- [ ] Use `GARBAGE_COLLECT_NEVER` for frequently spawned prefabs

### Phase 7: Platform-Specific

- [ ] **Dreamcast:** Reduce polygons, use DTEX/KMG textures, max 2 lights, avoid skyboxes
- [ ] **PSP:** Power-of-two textures, max 512px, WAV audio, reduce poly count
- [ ] **Desktop:** Use DDS textures, enable high-quality features conditionally
- [ ] Use `#ifdef SIMULANT_PLATFORM_*` for platform-specific assets
- [ ] Test on target hardware (not just desktop)

### Phase 8: Code Quality

- [ ] No allocations inside `on_update()` loops
- [ ] Cache node lookups (store pointers once in `on_load()`)
- [ ] Cache asset lookups (resolve ID to pointer once per frame, not per object)
- [ ] Connect signals once in `on_load()`, not in `on_update()`
- [ ] Use `ScopedConnection` for automatic cleanup

---

## Further Reading

- [Profiling](../profiling.md) -- Dreamcast profiler details
- [Partitioners & Culling](../partitioners.md) -- Spatial optimization
- [Asset Managers](../assets/asset-managers.md) -- Deep dive into asset lifecycle
- [Resource Management](../core-concepts/resource-management.md) -- IDs, scope blocks, GC
- [Physics Overview](../physics/overview.md) -- Body types, colliders, joints
- [Scene Management](../core-concepts/scene-management.md) -- Preloading, transitions
- [Coroutines](../coroutines.md) -- Async loading patterns
- [Signals](../scripting/signals.md) -- Event system performance
- [Actors](../actors.md) -- Actor vs Geom, detail levels
- [Spatial Hashing](../spatial_hashing.md) -- Partitioner details
