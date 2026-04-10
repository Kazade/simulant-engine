# StageNodes

StageNodes are the fundamental building blocks of every scene in Simulant. Every object you see on screen, every camera, every light, and every grouping construct in your scene is a StageNode or a subclass of one. Understanding how StageNodes work and how they form a scene graph is essential to building any game with Simulant.

**Related documentation:** [Actors](../actors.md), [Cameras](../cameras.md).

---

## 1. What is a StageNode?

`StageNode` is the abstract base class for all scene objects in Simulant. It combines several capabilities into one class:

- **Transform** -- position, rotation, and scale in 2D or 3D space.
- **Hierarchy** -- parent-child relationships that form a tree (the scene graph).
- **Update lifecycle** -- `on_update()`, `on_late_update()`, and `on_fixed_update()` callbacks every frame.
- **Naming** -- every node can be given a name for later lookup.
- **Visibility and culling** -- control over whether the node participates in rendering.
- **Bounds** -- an axis-aligned bounding box (AABB) for collision and frustum culling.
- **Renderable generation** -- the ability to produce renderables for the render queue.

Inheritance chain:

```
StageNode
  |-- TransformListener   (receives transform change notifications)
  |-- Updateable          (participates in the per-frame update cycle)
  |-- Nameable            (has a name property)
  |-- BoundableEntity     (has an AABB)
  |-- DestroyableObject   (safe destruction with signals)
  |-- Printable           (can be printed for debugging)
```

Every StageNode belongs to a `Scene`, which owns it. Nodes are managed through a pool and are not deleted immediately when you call `destroy()` -- cleanup is deferred to a safe point in the frame.

---

## 2. The Scene Graph Hierarchy

All StageNodes form a **tree structure** called the **scene graph**. At the root of each tree is a `Stage` node (a StageNode subclass that acts purely as a container). Every other node is either a direct child of the Stage or a descendent further down the tree.

```
Stage (root)
  |-- Actor "Player"
  |     |-- Actor "Weapon"
  |     |-- ParticleSystem "Trail"
  |     `-- Camera "FPS_Camera"
  |-- Stage "Environment"
  |     |-- Actor "Tree_1"
  |     |-- Actor "Tree_2"
  |     `-- Light "StreetLamp"
  |-- Camera "MainCamera"
  `-- Light "DirectionalLight"
```

### Why a tree?

- **Transform inheritance**: A child node's transform is always relative to its parent. Rotating the parent rotates all children. Moving the parent moves all children.
- **Culling**: If a parent is hidden (`set_visible(false)`), all children are hidden too.
- **Organized structure**: Grouping related objects under a common parent makes them easy to manage, move, and destroy together.

At the top level, every `Scene` owns a root `Stage`. You typically access it through `scene->stage()` or create additional stages for different rendering pipelines.

---

## 3. Built-in StageNode Types

Simulant ships with many StageNode subclasses. Here is a summary of the most commonly used ones:

### Core Types

| Type | Description |
|------|-------------|
| `Stage` | A container node with no mesh of its own. Used to group other nodes together. See [Stage](#2-the-scene-graph-hierarchy) above. |
| `Actor` | The primary visible object type. Holds a mesh and participates in rendering. See [Actors](../actors.md) for details. |
| `Camera` / `Camera2D` / `Camera3D` | Defines a viewpoint into the scene. See [Cameras](../cameras.md) for details. |
| `Light` / `DirectionalLight` / `PointLight` | Adds lighting to the scene. |

### Rendering and Effects

| Type | Description |
|------|-------------|
| `Geom` | A node that renders raw geometry data without needing a full Actor/mesh setup. Useful for debug drawing and procedural geometry. |
| `Sprite` | A lightweight 2D billboard that renders a texture. Ideal for UI elements, particles, and HUD objects. |
| `ParticleSystem` | Renders a particle effect defined by a particle script. |
| `Skybox` | Renders a skybox around the scene. |
| `MeshInstancer` | Efficiently renders many instances of the same mesh. |

### UI Widgets

| Type | Description |
|------|-------------|
| `WidgetButton` | A clickable button widget. |
| `WidgetLabel` | A text label. |
| `WidgetFrame` | A container frame for other widgets. |
| `WidgetProgressBar` | A progress bar widget. |
| `WidgetImage` | An image display widget. |
| `WidgetTextEntry` | A text input field. |
| `WidgetKeyboard` / `WidgetKeyboardPanel` | On-screen keyboard components. |
| `UIManager` | Manages the UI widget hierarchy. |

### Behaviors and Utilities

| Type | Description |
|------|-------------|
| `AnimationController` | Controls animation playback on actors. |
| `AudioSource` | A 3D sound source placed in the scene. |
| `Debug` | Debug visualization node. |
| `StatsPanel` | Displays performance statistics. |
| `PrefabInstance` | An instance of a prefab loaded from a `.gltf` or similar file. |
| `FlyController` | First-person fly-camera behavior. |
| `SmoothFollow` | Camera that smoothly follows a target. |
| `CylindricalBillboard` / `SphericalBillboard` | Makes a node rotate to always face the camera. |
| `PhysicsStaticBody` / `PhysicsDynamicBody` / `PhysicsKinematicBody` | Physics rigid body wrappers (implemented as Behaviors). |
| `PartitionerFrustum` / `PartitionerSpatialHash` | Scene partitioners for frustum culling. |

### Custom Types

You can define your own StageNode subclasses with a unique type ID starting at `STAGE_NODE_TYPE_USER_BASE` (value `1000`).

---

## 4. Creating Nodes

Nodes are created through the `Scene` or via the `create_child<T>()` method on an existing StageNode. There are two main approaches:

### Creating as a child of an existing node

```cpp
// Inside a Scene or any StageNode
auto actor = create_child<Actor>();
auto light = create_child<DirectionalLight>();
auto camera = create_child<Camera3D>();
```

`create_child<T>()` does three things:
1. Creates the node through the scene's node manager.
2. Sets the calling node as its parent.
3. Returns a pointer to the new node (or `nullptr` on failure).

### Creating with parameters

Nodes can be initialized with parameters at construction time. The parameters depend on the node type.

```cpp
// Create an actor with a specific mesh
auto actor = create_child<Actor>(my_mesh_id, DETAIL_LEVEL_NEAREST);

// Create a particle system with a script
auto particles = create_child<ParticleSystem>(particle_script);
```

You can also pass a `Params` object directly:

```cpp
Params params;
params.set("position", FloatArray{1.0f, 2.0f, 3.0f});
auto actor = create_child<Actor>(params);
```

### Creating a standalone node

When you need a node that is **not** yet attached as a child:

```cpp
// create_node creates a node in the scene's pool without setting a parent
auto node = scene->create_node<Actor>();
// You can then attach it manually:
node->set_parent(stage_);
```

### Naming at creation

Chain `set_name_and_get()` to name a node immediately after creation:

```cpp
auto player = create_child<Actor>()->set_name_and_get("Player");
auto enemy = create_child<Actor>()->set_name_and_get("Enemy");
```

---

## 5. Parent-Child Relationships

### Setting a parent

Use `set_parent()` to establish a parent-child relationship:

```cpp
auto gun = create_child<Actor>();
auto player = create_child<Actor>();

gun->set_parent(player);
// 'gun' is now a child of 'player'
// Moving 'player' will also move 'gun'
```

### Transform retain modes

When you change a node's parent, you can control whether it keeps its world-space transform:

```cpp
// Default: lose relative transform (node keeps its world position)
node->set_parent(new_parent, TRANSFORM_RETAIN_MODE_LOSE);

// Keep the relative transform (node may jump in world space)
node->set_parent(new_parent, TRANSFORM_RETAIN_MODE_KEEP);
```

### Detaching from a parent

Pass `nullptr` to detach a node from its parent. The node becomes a direct child of the Stage (a "stray" node):

```cpp
child->set_parent(nullptr);
// child is now parentless (belongs to the Stage root)
```

Or use `remove_from_parent()` for a more readable alternative:

```cpp
child->remove_from_parent();
```

### Detaching all children

To remove all children from a node at once, use `detach()`:

```cpp
std::list<StageNode*> orphans = parent->detach();
// 'orphans' contains all the former children
// 'parent' no longer has a parent either
```

### Reparenting

Reparenting is simply calling `set_parent()` with a different node. The node is automatically removed from its old parent's child list and appended to the new parent's child list:

```cpp
auto box = create_child<Actor>();
box->set_parent(container_a);

// Later...
box->set_parent(container_b);
// 'box' is now a child of container_b, no longer container_a
```

### Adopting children

The `adopt_children()` method is a convenience for adopting multiple nodes at once:

```cpp
adopt_children(node_a, node_b, node_c);
```

---

## 6. Tree Traversal

Simulant provides range-based iterators for traversing the scene graph. All of them work with C++ range-based `for` loops.

### Iterating direct children

```cpp
// Iterate only the immediate children of a node
for (auto& child : node->each_child()) {
    printf("Child: %s\n", child.name().c_str());
}
```

### Iterating all descendents (root-to-leaf)

```cpp
// Recursively visits every node in the subtree
for (auto& desc : stage->each_descendent()) {
    printf("Node: %s\n", desc.name().c_str());
}
```

### Iterating ancestors (up the tree)

```cpp
// Walk from this node up to the root
for (auto& ancestor : actor->each_ancestor()) {
    printf("Ancestor: %s\n", ancestor.name().c_str());
}
```

### Iterating siblings

```cpp
// Iterate all siblings (including self)
for (auto& sibling : actor->each_sibling()) {
    printf("Sibling: %s\n", sibling.name().c_str());
}
```

### Counting children

```cpp
size_t count = node->child_count();

// Access a specific child by index
const StageNode* first = node->child_at(0);
```

### Finding the node path

Every node can report its full path from the root as a `StageNodePath` object:

```cpp
StageNodePath path = node->node_path();
printf("Path: %s\n", path.to_string().c_str());
```

The path is a sequence of unique node IDs from the root to the target node.

---

## 7. Node Naming and Finding

### Setting a name

```cpp
actor->set_name("Player");

// Or name at creation time:
auto boss = create_child<Actor>()->set_name_and_get("Boss");
```

### Finding by name

`find_descendent_with_name()` recursively searches the subtree for a node with the given name. If multiple nodes share the same name, the first match is returned:

```cpp
StageNode* weapon = player->find_descendent_with_name("Weapon");
if (weapon) {
    // Found it
}
```

### Finding by unique ID

Every StageNode has a unique `StageNodeID`. You can look it up directly:

```cpp
StageNode* node = stage->find_descendent_with_id(some_id);
```

### Finding by type

Find all nodes of specific types within a subtree:

```cpp
std::vector<StageNode*> cameras = stage->find_descendents_by_types(
    {STAGE_NODE_TYPE_CAMERA3D, STAGE_NODE_TYPE_CAMERA2D}
);
```

### Counting nodes by type (debugging)

```cpp
// Warning: this uses dynamic_cast and is slow. Debug/testing only.
size_t actor_count = stage->count_nodes_by_type<Actor>();
```

---

## 8. Node Destruction

### Immediate destruction

Call `destroy()` to mark a node for destruction. The node is **not** deleted immediately -- cleanup is deferred until after `late_update()` but before the render queue is built. This prevents dangling pointers during the frame.

```cpp
enemy->destroy();
// enemy is still accessible this frame
// It will be cleaned up at the end of the frame
```

When `destroy()` is called:
1. The `signal_destroyed` signal fires immediately.
2. All child nodes are also queued for destruction.
3. All mixins attached to the node are queued for destruction.
4. Actual cleanup runs after `late_update()`.
5. Just before deletion, `signal_cleaned_up` fires.

### Immediate (synchronous) destruction

In rare cases where you need a node gone right now:

```cpp
node->destroy_immediately();
```

This should be used with caution, as it can invalidate pointers held elsewhere.

### Delayed destruction

Use `destroy_after(seconds)` to schedule destruction for a future time. This returns a `Promise<void>` that fulfills when destruction happens:

```cpp
// Destroy this projectile after 3 seconds
projectile->destroy_after(Seconds(3.0f));

// Or chain it:
auto temp = create_child<Actor>()->set_name_and_get("Temporary");
temp->destroy_after(Seconds(5.0f));
```

**Important:** `destroy_after()` is fire-and-forget. Once called, you cannot cancel it. Also, `is_marked_for_destruction()` will return `false` until the timer expires and `destroy()` is actually invoked.

---

## 9. Disabling Culling

By default, StageNodes (excluding cameras) are subject to frustum culling. If a node is determined to be offscreen, it will not be rendered. You can disable this per-node:

```cpp
// This node will always be rendered, regardless of camera view
node->set_cullable(false);

assert(!node->is_cullable());
```

This is useful for:
- HUD elements and overlays that must always appear.
- Debug visualizations that you need to see regardless of camera position.
- Audio sources or logic nodes that should not participate in spatial culling.

Note that cullability is separate from visibility (`set_visible()`). A non-cullable node that is invisible still will not render.

---

## 10. Mixins

Mixins let you attach additional behavior to a StageNode **without** adding extra nodes to the scene graph hierarchy. A mixin shares the same `Transform` as its base node, and both receive update callbacks.

### Creating a mixin

```cpp
auto actor = create_child<Actor>();

// Attach a custom behavior as a mixin
auto follow = actor->create_mixin<SmoothFollow>();
auto billboard = actor->create_mixin<CylindricalBillboard>();
```

In this example:
- `actor` is the node in the scene graph.
- `follow` and `billboard` are mixins attached to `actor`.
- All three share the same transform.
- All three receive `on_update()`, `on_late_update()`, and `on_fixed_update()` calls.

### Finding a mixin

```cpp
// By type
auto follow = actor->find_mixin<SmoothFollow>();

// By name
StageNode* mixin = actor->find_mixin("SmoothFollow");
```

### Mixin rules

- Mixins **cannot** be nested. You can only attach mixins to a base StageNode, not to another mixin.
- You cannot create duplicate mixins of the same type on one base node.
- A mixin's `is_mixin()` returns `true`; the base node's `is_mixin()` returns `false`.
- Access the base node from a mixin via `mixin->base()`.
- When the base node is destroyed, all its mixins are destroyed too.

### When to use mixins

Use mixins when you want to compose behavior without deepening the scene graph. For example, instead of creating a separate `SmoothFollow` node and making the camera its child, attach `SmoothFollow` as a mixin to the camera itself. This keeps the hierarchy flat and easier to reason about.

---

## 11. Finding Dependent Nodes (FindDescendent, FindAncestor)

When writing custom StageNode or Behavior classes, you often need references to related nodes (e.g., a `CarBehavior` needs access to its child wheel nodes). The `FindResult<T>` helpers provide a clean, cached way to do this.

### FindDescendent

Searches **down** the tree from the given node:

```cpp
class CarBehavior : public StageNode {
public:
    // These are members declared in your class body
    FindResult<Actor> front_left_wheel = FindDescendent("Front Left", this);
    FindResult<Actor> front_right_wheel = FindDescendent("Front Right", this);
    FindResult<Actor> rear_left_wheel = FindDescendent("Rear Left", this);
    FindResult<Actor> rear_right_wheel = FindDescendent("Rear Right", this);

    void on_update(float dt) override {
        // Access works like a pointer; search is performed on first access
        if (front_left_wheel) {
            front_left_wheel->rotate(dt);
        }
    }
};
```

### FindAncestor

Searches **up** the tree for a named ancestor:

```cpp
class WheelBehavior : public StageNode {
public:
    // Find the nearest ancestor named "CarBody"
    FindResult<Actor> car_body = FindAncestor("CarBody", this);

    void on_update(float dt) override {
        if (car_body) {
            // Use car_body...
        }
    }
};
```

### How FindResult works

- **Lazy evaluation**: The actual search does not happen until you first access the result (e.g., `front_left_wheel->`).
- **Caching**: Once found, the result is cached for subsequent accesses.
- **Automatic invalidation**: If the found node is destroyed, the cache is cleared and the next access will search again.
- **Scene notifications**: The result subscribes to relevant scene change notifications so it knows when to invalidate its cache.

You can also check if a result is cached without triggering a search:

```cpp
if (front_left_wheel.is_cached()) {
    // A search has already been performed
}
```

### Other finders

```cpp
// Find a child node by type (immediate children only)
FindResult<Actor> wheel = FindChild<Actor>(this);

// Find a mixin by type
FindResult<SmoothFollow> follow = FindMixin<SmoothFollow>(this);

// Find by unique ID
FindResult<StageNode> node = FindDescendentByID(some_id, this);
```

---

## 12. The Update Cycle

### When nodes get updated

StageNodes participate in the per-frame update cycle **only if** their owning Stage is attached to an active render pipeline (Layer). You can check this condition:

```cpp
if (node->is_part_of_active_pipeline()) {
    // This node will be updated this frame
}
```

### Update order

Each frame, the engine calls three update methods in order:

1. **`on_update(float dt)`** -- Main update. Called first on all nodes.
2. **`on_fixed_update(float step)`** -- Physics/fixed-timestep update. Called at a fixed rate.
3. **`on_late_update(float dt)`** -- Late update. Called after all `on_update()` calls have completed.

Within each phase, the traversal order is: the node itself, then its mixins, then its children (recursively).

```
for each node in tree:
    node->on_update(dt)
    for each mixin: mixin->on_update(dt)
    for each child: recurse
```

The same pattern applies for `fixed_update` and `late_update`.

### The `active_pipeline_count_`

A node's `is_part_of_active_pipeline()` returns `true` when `active_pipeline_count_` is greater than zero. This count is managed by the Layer system when stages are added to or removed from render pipelines.

### Destroyed nodes do not update

If a node has been marked for destruction, all its update methods return immediately without doing any work.

---

## 13. Best Practices for Organizing Scene Graphs

### 1. Use `Stage` nodes as logical groupings

Group related objects under named `Stage` nodes rather than attaching everything directly to the root:

```cpp
auto environment = create_child<Stage>()->set_name_and_get("Environment");
auto vehicles = create_child<Stage>()->set_name_and_get("Vehicles");
auto ui = create_child<Stage>()->set_name_and_get("UI");

// Then populate each group
tree->set_parent(environment);
car->set_parent(vehicles);
```

### 2. Keep the hierarchy shallow when possible

Deep hierarchies make traversal expensive and harder to reason about. Use **mixins** to compose behavior without adding depth.

**Avoid this:**
```
Camera
  |-- FollowBehavior
        |-- LookAtController
              |-- Smoothing
```

**Prefer this:**
```
Camera (with SmoothFollow mixin, LookAtController mixin)
```

### 3. Parent objects that move together

If a weapon is always in the player's hand, make it a child of the player. This way you only ever need to move the player, and the weapon follows automatically.

### 4. Use meaningful names

Always name nodes that you intend to look up later. This makes debugging and `find_descendent_with_name()` calls reliable:

```cpp
auto spawn_point = create_child<Actor>()->set_name_and_get("EnemySpawn_01");
```

### 5. Destroy entire subtrees by destroying the root

Calling `destroy()` on a parent automatically queues all children and mixins for destruction. You do not need to clean up children manually:

```cpp
// This destroys the stage and everything under it
environment_stage->destroy();
```

### 6. Disable culling for always-visible nodes

Nodes that must always render (like HUD elements) should have culling disabled:

```cpp
hud_element->set_cullable(false);
```

### 7. Be cautious with `destroy_after()`

Because `destroy_after()` cannot be cancelled, use it only for objects with a natural lifetime (projectiles, temporary effects, etc.). For objects whose lifetime depends on game logic, use explicit `destroy()` calls instead.

### 8. Use `find_descendent_with_name` sparingly in hot paths

Name-based lookup is O(n) in the size of the subtree. For lookups needed every frame, prefer `FindResult<T>` members that cache their results, or store direct pointers.

### 9. Prefer `set_name_and_get()` for one-liners

```cpp
// Good -- creates and names in one expression
auto player = create_child<Actor>()->set_name_and_get("Player");

// Also fine -- two steps
auto enemy = create_child<Actor>();
enemy->set_name("Enemy");
```

### 10. Use `detach()` carefully

Calling `detach()` on a node removes **all** of its children and also detaches the node from its own parent. Make sure you have a plan for the orphaned nodes.

---

## Summary

| Concept | Key Methods |
|---------|-------------|
| Creating nodes | `create_child<T>()`, `set_name_and_get()` |
| Parent-child | `set_parent()`, `remove_from_parent()`, `detach()` |
| Traversal | `each_child()`, `each_descendent()`, `each_ancestor()`, `each_sibling()` |
| Finding by name | `find_descendent_with_name()` |
| Finding dependents | `FindDescendent`, `FindAncestor`, `FindResult<T>` |
| Destruction | `destroy()`, `destroy_after()`, `destroy_immediately()` |
| Culling | `set_cullable()`, `is_cullable()` |
| Visibility | `set_visible()`, `is_visible()` |
| Mixins | `create_mixin<T>()`, `find_mixin<T>()` |
| Update check | `is_part_of_active_pipeline()` |
| Bounds | `aabb()`, `transformed_aabb()` |
