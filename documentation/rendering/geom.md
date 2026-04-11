# Geom -- Optimized Static Geometry

This guide covers the `Geom` node, Simulant's mechanism for rendering static, immovable mesh geometry with high performance. Geoms are ideal for level geometry, terrain, buildings, and any mesh that does not need to be moved, rotated, or animated at runtime.

**See also:** [Meshes](meshes.md), [Mesh Instancer](mesh-instancer.md), [Actors](../core-concepts/actors.md), [Render Pipelines](pipelines.md), [Partitioners](../partitioners.md)

---

## Table of Contents

1. [What Is a Geom?](#1-what-is-a-geom)
2. [Geom vs Actor vs MeshInstancer](#2-geom-vs-actor-vs-meshinstancer)
3. [Creating a Geom](#3-creating-a-geom)
4. [How Geom Works Internally](#4-how-gom-works-internally)
5. [Octree Culler](#5-octree-culler)
6. [Quadtree Culler](#6-quadtree-culler)
7. [Configuring the Culler](#7-configuring-the-culler)
8. [Two-Phase Culling](#8-two-phase-culling)
9. [Performance Characteristics](#9-performance-characteristics)
10. [Converting Static Actors to Geoms](#10-converting-static-actors-to-geoms)
11. [Complete Example: Level Geometry Scene](#11-complete-example-level-geometry-scene)
12. [Limitations and Warnings](#12-limitations-and-warnings)
13. [Best Practices](#13-best-practices)

---

## 1. What Is a Geom?

A `Geom` is a `StageNode` that renders a mesh as **static, immovable geometry**. Unlike an `Actor`, a Geom cannot be translated, rotated, or scaled after creation. This immutability gives the engine significant freedom to optimize rendering:

- **Pre-transformed vertices**: The mesh vertices are transformed once at creation time and stored in a dedicated vertex buffer. No per-frame transform is needed during rendering.
- **Spatial subdivision**: The mesh is compiled into an internal octree or quadtree structure, enabling fine-grained frustum culling at the triangle-group level.
- **Reduced overhead**: Geoms skip animation updates, material slot switching, detail-level selection, and other Actor features that are irrelevant for static geometry.

A Geom requires a `MeshPtr` at creation time -- unlike an Actor, a Geom without a mesh is invalid.

---

## 2. Geom vs Actor vs MeshInstancer

Simulant offers three ways to render meshes in a scene. Understanding their trade-offs is essential for good rendering performance:

| Feature | `Actor` | `Geom` | `MeshInstancer` |
|---------|---------|--------|-----------------|
| Movable at runtime | Yes | No | No (instances are fixed) |
| Per-instance transform | Yes | N/A (one transform at creation) | Yes (many instances) |
| Animation support | Yes (skeletal, keyframe) | No | No |
| Material slots | Yes (8 slots) | No | No |
| Detail levels (LOD) | Yes | No | No |
| Spatial subdivision culling | No (broadphase only) | Yes (octree/quadtree) | No |
| Memory overhead per instance | Higher | N/A (single node) | Low (GPU instancing) |
| Best for | Dynamic objects, characters | Level geometry, terrain | Foliage, rocks, repeated props |

**Rule of thumb:**
- Use **Actor** for anything that moves, animates, or swaps materials
- Use **Geom** for static level geometry that never changes
- Use **MeshInstancer** for many copies of the same mesh (trees, grass, debris)

---

## 3. Creating a Geom

A Geom is created as a child of a `Scene` or any `StageNode`. It requires a mesh and accepts optional transform parameters:

```cpp
// Load or create the mesh
auto level_mesh = assets->load_mesh("levels/level_01.obj");

// Create the Geom -- position, orientation, and scale are applied once
auto level_geom = create_child<smlt::Geom>(
    smlt::Params()
        .set("mesh", level_mesh)
        .set("position", smlt::Vec3(0, 0, 0))
        .set("orientation", smlt::Quaternion())
        .set("scale", smlt::Vec3(1, 1, 1))
);
```

### Setting Render Priority

Like Actors, Geoms have a render priority that controls draw order:

```cpp
level_geom->set_render_priority(smlt::RENDER_PRIORITY_DISTANT);
```

### Accessing the AABB

A Geom exposes an axis-aligned bounding box for use by the scene partitioner:

```cpp
const smlt::AABB& bounds = level_geom->aabb();
```

---

## 4. How Geom Works Internally

When a Geom is created, it goes through a compilation phase:

1. **Mesh validation**: The Geom verifies that a valid mesh was provided
2. **Culler selection**: Based on `GeomCullerOptions`, either an `OctreeCuller` or `QuadtreeCuller` is created
3. **Vertex transformation**: The mesh's vertex data is cloned and transformed by the Geom's position, orientation, and scale. This transformed copy is owned by the culler.
4. **Spatial subdivision**: The culler subdivides the transformed mesh into its internal tree structure, grouping triangles by spatial region and material
5. **Compilation**: The tree is marked as compiled and is ready for frustum queries

At render time, the Geom's `do_generate_renderables()` method queries its culler for visible regions and inserts the resulting `Renderable` objects into the render queue. No per-frame transform calculations are needed because the vertices are already in world space.

```cpp
// From Geom::do_generate_renderables():
void Geom::do_generate_renderables(batcher::RenderQueue* render_queue,
                                   const Camera* camera, const Viewport* viewport,
                                   const DetailLevel detail_level,
                                   Light** lights, const std::size_t light_count) {
    // The culler handles all the work -- just ask it for visible renderables
    culler_->renderables_visible(camera->frustum(), render_queue);
}
```

---

## 5. Octree Culler

The `OctreeCuller` is the default culler type. It subdivides the mesh's bounding volume into an octree -- a hierarchical 3D space partition where each node has up to 8 children.

### How It Works

1. **Bounds calculation**: The octree is sized to encompass the mesh's transformed AABB
2. **Recursive subdivision**: The space is subdivided up to `max_depth` levels. At each level, a node splits into 8 children (one for each octant)
3. **Triangle assignment**: Each triangle from every submesh is assigned to the deepest octree node that fully contains it. Triangles are grouped by material within each node
4. **Frustum traversal**: At render time, the octree is traversed. Nodes outside the camera frustum are skipped entirely. Nodes that pass contribute their grouped triangles as `Renderable` objects

### Constructor

```cpp
OctreeCuller(Geom* geom, const MeshPtr mesh, uint8_t max_depth);
```

### Default Maximum Depth

The default `octree_max_depth` is **4**. This produces up to 8^4 = 4,096 leaf nodes in the worst case, though in practice many branches terminate early if they contain no triangles.

### Querying Bounds

```cpp
smlt::AABB bounds = octree_culler->octree_bounds();
```

### When to Use Octree

- **Enclosed 3D environments**: indoor levels, dungeons, buildings
- **Complex 3D meshes**: meshes with geometry distributed in all three axes
- **General-purpose static geometry**: the safe default choice

---

## 6. Quadtree Culler

The `QuadtreeCuller` subdivides space in 2D (4 children per node instead of 8). It is designed for terrain and other geometry that is predominantly flat on one axis.

### How It Works

The quadtree operates similarly to the octree but splits each node into 4 children along a 2D plane (typically the XZ plane for terrain). This is more memory-efficient than an octree for terrain because there is no meaningful subdivision along the vertical axis.

### Constructor

```cpp
QuadtreeCuller(Geom* geom, const MeshPtr mesh, uint8_t max_depth);
```

### Default Maximum Depth

The default `quadtree_max_depth` is **4**, producing up to 4^4 = 256 leaf nodes.

### When to Use Quadtree

- **Terrain meshes**: heightmap-generated or sculpted terrain
- **Large flat surfaces**: outdoor environments where vertical subdivision provides no benefit
- **Very large meshes**: where octree subdivision would create too many empty nodes

---

## 7. Configuring the Culler

The `GeomCullerOptions` struct controls which culler type is used and its maximum subdivision depth:

```cpp
struct GeomCullerOptions {
    GeomCullerType type = GEOM_CULLER_TYPE_OCTREE;
    uint8_t octree_max_depth = 4;
    uint8_t quadtree_max_depth = 4;
};
```

| Property | Default | Description |
|----------|---------|-------------|
| `type` | `GEOM_CULLER_TYPE_OCTREE` | The culler type: `GEOM_CULLER_TYPE_OCTREE` or `GEOM_CULLER_TYPE_QUADTREE` |
| `octree_max_depth` | `4` | Maximum subdivision depth for octrees (0-8 recommended) |
| `quadtree_max_depth` | `4` | Maximum subdivision depth for quadtrees (0-8 recommended) |

### Passing Options at Creation

```cpp
smlt::GeomCullerOptions opts;
opts.type = smlt::GEOM_CULLER_TYPE_OCTREE;
opts.octree_max_depth = 5;  // Deeper subdivision for complex indoor level

auto level = create_child<smlt::Geom>(
    smlt::Params()
        .set("mesh", level_mesh)
        .set("position", smlt::Vec3(0, 0, 0))
        .set("options", opts)
);
```

### Choosing the Right Depth

- **Depth 0-2**: Minimal subdivision. Suitable for small meshes where culling overhead outweighs benefits
- **Depth 3-4**: Balanced default. Good for most level-sized meshes
- **Depth 5-6**: Fine-grained subdivision. Useful for very large or complex meshes
- **Depth 7+**: Usually excessive. Increases memory usage and compilation time with diminishing returns

Higher depth means more precise culling but also more memory for the tree structure and longer compilation time at Geom creation.

---

## 8. Two-Phase Culling

Geom participates in a **two-phase culling pipeline** that maximizes rendering efficiency:

### Phase 1: Scene Partitioner (Broadphase)

The scene's partitioner (e.g., loose octree, spatial hash) tracks all `StageNode` objects including Geoms. During rendering, it determines which nodes are potentially visible from the camera's viewpoint. If a Geom's AABB is entirely outside the frustum, it is skipped at this stage -- no further processing occurs.

### Phase 2: Geom Culler (Narrowphase)

If the partitioner determines that a Geom is potentially visible, the Geom's internal culler (octree or quadtree) performs a second, more detailed frustum test. Only the specific tree nodes that intersect the camera frustum contribute renderables. This means that even if only a small portion of a large level mesh is visible, only that portion is rendered.

```
Camera frustum test
        |
        v
[Scene Partitioner] -- culls entire Geom nodes by AABB
        |
        v (if potentially visible)
[Geom Culler Octree/Quadtree] -- culls individual triangle groups
        |
        v
Visible Renderables --> Render Queue
```

This two-phase approach provides excellent performance: cheap broadphase rejection eliminates obviously invisible Geoms entirely, while the narrowphase culler handles partial visibility within large meshes.

---

## 9. Performance Characteristics

### Advantages Over Actor

| Metric | Actor | Geom |
|--------|-------|------|
| Per-frame transform | Yes (matrix multiply) | No (pre-transformed) |
| Per-frame animation update | Yes | No |
| Frustum culling granularity | Whole mesh (AABB) | Per-tree-node (triangle groups) |
| Material batching | Per-submesh | Per-material-per-tree-node |
| Memory per instance | Full actor overhead | Culler tree structure |

### When Geom Shines

- **Large level meshes**: A single 100,000-triangle level mesh loaded as a Geom only renders the visible fraction. As an Actor, the entire mesh is submitted to the renderer every frame (even if the renderer clips it later, the CPU overhead is still paid)
- **Multiple static props**: Walls, floors, ceilings, and other immutable structures
- **Complex terrain**: Large outdoor areas where only a portion is on-screen at any time

### When Geom Does Not Help

- **Small meshes**: A single cube or sphere gains almost nothing from subdivision culling. An Actor is simpler and equally fast
- **Fully visible meshes**: If the entire mesh is always on screen (e.g., a small room filling the view), subdivision culling provides no benefit
- **Frequently changing meshes**: Geom compilation is a one-time cost. If you need to modify vertices, use an Actor

---

## 10. Converting Static Actors to Geoms

If you have existing code that uses Actors for static geometry, converting to Geoms is straightforward:

### Before: Static Actors (Slower)

```cpp
void on_load() override {
    // Each wall is a separate Actor -- unnecessary overhead
    auto wall_mesh = assets->load_mesh("models/wall.obj");

    auto wall1 = create_child<smlt::Actor>(wall_mesh);
    wall1->transform->set_position(smlt::Vec3(10, 0, 0));

    auto wall2 = create_child<smlt::Actor>(wall_mesh);
    wall2->transform->set_position(smlt::Vec3(-10, 0, 0));

    auto floor = create_child<smlt::Actor>(
        assets->load_mesh("models/floor.obj")
    );
    floor->transform->set_position(smlt::Vec3(0, -1, 0));

    auto ceiling = create_child<smlt::Actor>(
        assets->load_mesh("models/ceiling.obj")
    );
    ceiling->transform->set_position(smlt::Vec3(0, 10, 0));
}
```

### After: Geoms (Faster)

```cpp
void on_load() override {
    // Each wall is a Geom -- optimized static rendering
    auto wall_mesh = assets->load_mesh("models/wall.obj");

    auto wall1 = create_child<smlt::Geom>(
        smlt::Params()
            .set("mesh", wall_mesh)
            .set("position", smlt::Vec3(10, 0, 0))
    );

    auto wall2 = create_child<smlt::Geom>(
        smlt::Params()
            .set("mesh", wall_mesh)
            .set("position", smlt::Vec3(-10, 0, 0))
    );

    auto floor = create_child<smlt::Geom>(
        smlt::Params()
            .set("mesh", assets->load_mesh("models/floor.obj"))
            .set("position", smlt::Vec3(0, -1, 0))
    );

    auto ceiling = create_child<smlt::Geom>(
        smlt::Params()
            .set("mesh", assets->load_mesh("models/ceiling.obj"))
            .set("position", smlt::Vec3(0, 10, 0))
    );
}
```

### Single Combined Geom (Best for Level Geometry)

Even better, combine all static geometry into a single mesh and load it as one Geom:

```cpp
void on_load() override {
    // One mesh containing the entire level, compiled once
    auto level_mesh = assets->load_mesh("levels/level_01.obj");

    smlt::GeomCullerOptions opts;
    opts.type = smlt::GEOM_CULLER_TYPE_OCTREE;
    opts.octree_max_depth = 5;  // Fine subdivision for a large level

    auto level = create_child<smlt::Geom>(
        smlt::Params()
            .set("mesh", level_mesh)
            .set("position", smlt::Vec3(0, 0, 0))
            .set("options", opts)
    );
}
```

This is the most efficient approach: the octree culler handles visibility at a fine granularity, and you only have one node to manage.

---

## 11. Complete Example: Level Geometry Scene

A complete scene demonstrating Geom-based level rendering with a player camera:

```cpp
#include "simulant/simulant.h"

using namespace smlt;

class LevelScene : public Scene {
public:
    LevelScene(Window* window) : Scene(window) {}

    void on_load() override {
        // ---- Camera ----
        camera_ = create_child<Camera3D>();
        camera_->set_perspective_projection(
            Degrees(60.0f),
            float(window->width()) / float(window->height()),
            0.1f, 500.0f
        );
        camera_->transform->set_position(0, 5, -20);
        camera_->transform->look_at(0, 2, 0);

        // ---- Render Pipeline ----
        auto layer = compositor->create_layer(this, camera_)
            ->set_clear_flags(BUFFER_CLEAR_ALL)
            ->set_viewport(Viewport(VIEWPORT_TYPE_FULL, Color(0.3f, 0.4f, 0.6f, 1.0f)))
            ->set_priority(RENDER_PRIORITY_MAIN)
            ->set_name("level_render");
        link_pipeline(layer);

        // ---- Lighting ----
        lighting->set_ambient_light(Color(0.2f, 0.2f, 0.25f, 1.0f));

        auto sun = create_child<DirectionalLight>(
            Params()
                .set("direction", Vec3(1, -1.5f, 0.5f))
                .set("color", Color(1, 0.95f, 0.8f, 1))
        );

        // ---- Level Geometry ----
        // Option A: Single combined level mesh (recommended)
        auto level_mesh = assets->load_mesh("levels/dungeon_01.obj");

        GeomCullerOptions level_opts;
        level_opts.type = GEOM_CULLER_TYPE_OCTREE;
        level_opts.octree_max_depth = 5;

        level_geom_ = create_child<Geom>(
            Params()
                .set("mesh", level_mesh)
                .set("position", Vec3(0, 0, 0))
                .set("options", level_opts)
        );
        level_geom_->set_render_priority(RENDER_PRIORITY_DISTANT);

        // Option B: Individual props as separate Geoms
        auto pillar_mesh = assets->load_mesh("models/stone_pillar.obj");
        for (int i = 0; i < 4; ++i) {
            create_child<Geom>(
                Params()
                    .set("mesh", pillar_mesh)
                    .set("position", Vec3(-6 + i * 4, 0, 5))
            );
        }
    }

    void on_update(float dt) override {
        Scene::on_update(dt);
        // Camera movement for testing
        float speed = 10.0f * dt;
        if (input->axis_value("move_forward") > 0) {
            camera_->transform->translate(Vec3(0, 0, speed));
        }
        if (input->axis_value("move_backward") > 0) {
            camera_->transform->translate(Vec3(0, 0, -speed));
        }
    }

private:
    CameraPtr camera_;
    GeomPtr level_geom_;
};
```

---

## 12. Limitations and Warnings

### 1. Do Not Modify Mesh Vertex Data After Geom Creation

This is the most important rule. Once a Geom is created, **do not modify the mesh's vertex data**. The Geom's culler clones and transforms the vertex data at creation time. Changes to the original mesh's vertex positions or vertex count will cause visual corruption or crashes.

```cpp
auto mesh = assets->load_mesh("level.obj");
auto geom = create_child<Geom>(Params().set("mesh", mesh));

// DANGER: This will corrupt the Geom's internal data
mesh->vertex_data->set_position(0, Vec3(1, 2, 3));  // DO NOT DO THIS

// These might be safe (modifying attributes, not positions):
// mesh->vertex_data->set_normal(...)
// mesh->vertex_data->set_texcoord(...)
```

If you need to modify vertex positions at runtime, use an `Actor` instead.

### 2. No Runtime Movement

A Geom's transform is applied once at creation time during culler compilation. You cannot move, rotate, or scale a Geom after it has been created. If you need to animate or reposition geometry, use an `Actor`.

### 3. No Material Slots

Geoms do not support material slot switching. Each submesh uses its assigned material. If you need to swap materials on the same mesh for different instances, use an `Actor` with material slots or a `MeshInstancer`.

### 4. No Level of Detail (LOD)

Unlike Actors, Geoms do not support multiple detail levels. The full-resolution mesh is always used for visible regions. For LOD with static geometry, consider using separate Geoms at different distances or manually swapping meshes.

### 5. No Animation

Geoms do not support skeletal animation, keyframe animation, or any form of vertex animation. Animated objects must use `Actor`.

---

## 13. Best Practices

### 1. Combine Static Geometry into Few Large Meshes

A single Geom with a 50,000-triangle mesh and an octree culler is faster than 50 Geoms with 1,000 triangles each. Fewer nodes means less broadphase culling overhead and better material batching. Export your level as one combined mesh when possible.

### 2. Use Octree for General 3D, Quadtree for Terrain

```cpp
// Indoor level, building, dungeon
GeomCullerOptions opts;
opts.type = GEOM_CULLER_TYPE_OCTREE;
opts.octree_max_depth = 5;

// Outdoor terrain
GeomCullerOptions opts;
opts.type = GEOM_CULLER_TYPE_QUADTREE;
opts.quadtree_max_depth = 5;
```

### 3. Tune Culler Depth to Mesh Size

- Small props (< 1,000 triangles): depth 2-3 or just use Actor
- Rooms and corridors (1,000-10,000 triangles): depth 3-4
- Large levels (10,000-100,000 triangles): depth 4-5
- Massive terrain (100,000+ triangles): depth 5-6 with quadtree

### 4. Set Appropriate Render Priority

Static level geometry should typically render at `RENDER_PRIORITY_MAIN` or `RENDER_PRIORITY_DISTANT`. This ensures it renders before dynamic objects and UI overlays:

```cpp
level_geom->set_render_priority(smlt::RENDER_PRIORITY_DISTANT);
```

### 5. Use Directional Lights for Level Illumination

Static geometry is best lit with a small number of directional and point lights. Avoid placing dozens of point lights near large Geoms, as each renderable within the Geom will consider all lights in range during light sorting.

### 6. Keep the Mesh's Original Vertex Data Immutable

Since the Geom clones vertex data at creation, ensure the source mesh is fully loaded and finalized before creating the Geom. Do not use meshes that are still being modified by async loading operations.

### 7. Consider MeshInstancer for Repeated Props

If you need many copies of the same static prop (pillars, crates, barrels), use a `MeshInstancer` rather than creating individual Geoms. MeshInstancer uses GPU instancing which is far more efficient for repeated geometry:

```cpp
// Instead of 100 separate Geoms for trees:
auto instancer = create_child<MeshInstancer>(tree_mesh);
for (int i = 0; i < 100; ++i) {
    instancer->add_instance(
        tree_positions[i],  // position
        tree_rotations[i]   // orientation
    );
}
```
