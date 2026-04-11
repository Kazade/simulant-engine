# Behaviours

Behaviours are reusable components of game logic that you attach to nodes in the scene graph. They are the primary way to implement gameplay functionality in Simulant -- from player controllers and enemy AI to trigger zones and object pooling.

**Related documentation:** [Stage Nodes](../core-concepts/stage-nodes.md), [Physics](../physics/overview.md), [Signals](signals.md).

---

## Table of Contents

1. [What Are Behaviours and Why Use Them?](#1-what-are-behaviours-and-why-use-them)
2. [Behaviour Base Class and Lifecycle](#2-behaviour-base-class-and-lifecycle)
3. [Creating Custom Behaviours](#3-creating-custom-behaviours)
4. [Attaching Behaviours to StageNodes](#4-attaching-behaviours-to-stagenodes)
5. [The Behaviour Update Cycle](#5-the-behaviour-update-cycle)
6. [Physics Behaviours](#6-physics-behaviours)
7. [Physics Service Management](#7-physics-service-management)
8. [Finding Other Behaviours and Nodes](#8-finding-other-behaviours-and-nodes)
9. [Communication Between Behaviours](#9-communication-between-behaviours)
10. [Common Behaviour Patterns](#10-common-behaviour-patterns)
11. [Behaviour Best Practices](#11-behaviour-best-practices)
12. [Performance Considerations](#12-performance-considerations)
13. [Complete Examples](#13-complete-examples)

---

## 1. What Are Behaviours and Why Use Them?

In Simulant, **every node in the scene graph is a `StageNode`** (see [Stage Nodes](../core-concepts/stage-nodes.md)). A "behaviour" is simply a `StageNode` subclass that implements game logic rather than rendering. This includes physics bodies, camera controllers, animation controllers, and any custom gameplay component you write.

### Component-Based Design

Instead of building deep inheritance hierarchies like this:

```
GameObject
  |-- MovingGameObject
  |     |-- Enemy
  |     |     |-- FlyingEnemy
  |     |     `-- GroundEnemy
  |     `-- Player
  `-- StaticGameObject
        |-- Wall
        `-- Decoration
```

Simulant encourages a **composition-based approach** where you attach independent behaviour nodes to visual nodes:

```
Actor "Player" (visual)
  |-- DynamicBody (physics)
  |-- PlayerController (custom behaviour)
  |-- AudioSource (audio)
  `-- AnimationController (animation)
```

### Benefits

| Benefit | Explanation |
|---------|-------------|
| **Reusability** | A `HealthBehaviour` can be attached to players, enemies, or destructible objects without code duplication. |
| **Flexibility** | Add or remove capabilities at runtime by attaching or destroying behaviour nodes. |
| **Testability** | Individual behaviours can be tested in isolation. |
| **Clear ownership** | Each behaviour has one clear responsibility. A physics body handles collisions; a controller handles input. |
| **Scene graph integration** | Behaviours are full `StageNode` instances, so they receive update callbacks, can be found by name, and are cleaned up automatically. |

---

## 2. Behaviour Base Class and Lifecycle

All behaviours inherit from `StageNode`. The lifecycle consists of several virtual methods that the engine calls at specific times:

```cpp
class MyBehaviour : public StageNode {
public:
    S_DEFINE_STAGE_NODE_META(STAGE_NODE_TYPE_USER_BASE + 1, "my_behaviour");

    MyBehaviour(Scene* owner) : StageNode(owner, STAGE_NODE_TYPE_USER_BASE + 1) {}

protected:
    // Called when the node is created and parameters are validated
    bool on_create(Params params) override {
        // Initialise member variables, find child nodes, connect signals
        return true;  // Return false to abort creation
    }

    // Called every frame (variable timestep)
    void on_update(float dt) override {
        // Main game logic: input, movement, AI decisions
    }

    // Called at a fixed rate (physics timestep)
    void on_fixed_update(float step) override {
        // Physics-related logic that needs a consistent timestep
    }

    // Called after all on_update() calls have completed
    void on_late_update(float dt) override {
        // Camera follow, animation blending, anything that needs
        // all other objects to have finished their updates first
    }

    // Called when the node's parent changes
    void on_parent_set(const StageNode* old_parent, const StageNode* new_parent) override {
        // React to reparenting
    }

    // Called when the node is marked for destruction
    bool on_destroy() override {
        // Clean up resources, disconnect signals
        return true;  // Return true to proceed with destruction
    }
};
```

### Lifecycle Order

```
[Node Created]
    |
    v
on_create(params)          <-- Initialisation
    |
    v
on_update(dt)              <-- Per-frame logic (variable dt)
    |
    v
on_fixed_update(step)      <-- Fixed-timestep logic (constant step)
    |
    v
on_late_update(dt)         <-- Post-update logic (camera follow, etc.)
    |
    v
[Repeat each frame]
    |
    v
on_destroy()               <-- Cleanup when destroy() is called
    |
    v
[Node Deleted]
```

### Updateable Signals

In addition to overriding the virtual methods, you can also connect to signals that fire at the same times. Both mechanisms work and can be used together:

```cpp
// In on_create():
signal_update().connect([](float dt) {
    S_DEBUG("Update signal fired, dt = {}", dt);
});
```

See [Signals](signals.md) for full details on the signal system.

---

## 3. Creating Custom Behaviours

### Step 1: Define Your Behaviour Class

Create a header file for your behaviour. Every custom behaviour needs:

1. A unique type ID (starting from `STAGE_NODE_TYPE_USER_BASE`).
2. A metadata string for prefab/serialization support.
3. A constructor that takes `Scene*`.

```cpp
// behaviours/health_behaviour.h
#pragma once

#include <simulant/simulant.h>

namespace smlt {

class HealthBehaviour : public StageNode {
public:
    // Register this type with the engine
    S_DEFINE_STAGE_NODE_META(STAGE_NODE_TYPE_USER_BASE + 1, "health_behaviour");

    // Node parameters (optional, used for prefab support)
    S_DEFINE_STAGE_NODE_PARAM(HealthBehaviour, "max_health", float, 100.0f,
                              "Maximum health value");

    HealthBehaviour(Scene* owner)
        : StageNode(owner, STAGE_NODE_TYPE_USER_BASE + 1) {}

    // Public API for other behaviours to call
    void take_damage(float amount);
    void heal(float amount);
    float current_health() const { return current_health_; }
    bool is_alive() const { return current_health_ > 0.0f; }

    // Signals for other behaviours to listen to
    DEFINE_SIGNAL(sig::signal<void(float)>, signal_health_changed);
    DEFINE_SIGNAL(sig::signal<void()>, signal_died);

protected:
    bool on_create(Params params) override;
    void on_update(float dt) override;

private:
    float current_health_ = 100.0f;
    float max_health_ = 100.0f;
};

} // namespace smlt
```

### Step 2: Register Your Behaviour

Custom node types must be registered with the scene before you can create them. Do this in your scene's `on_load()` method or during application startup:

```cpp
void GameScene::on_load() {
    // Register custom behaviour types
    register_stage_node<HealthBehaviour>();
    register_stage_node<PlayerController>();
    register_stage_node<EnemyAI>();

    // Now you can use them
    auto player = create_child<Actor>();
    auto health = player->create_child<HealthBehaviour>();
}
```

### Step 3: Implement the Behaviour

```cpp
// behaviours/health_behaviour.cpp
#include "health_behaviour.h"

namespace smlt {

bool HealthBehaviour::on_create(Params params) {
    if (!clean_params<HealthBehaviour>(params)) {
        return false;
    }

    // Read parameters if provided
    if (params.contains("max_health")) {
        max_health_ = params.get<float>("max_health");
        current_health_ = max_health_;
    }

    return true;
}

void HealthBehaviour::on_update(float dt) {
    // Example: regenerate health over time
    if (current_health_ < max_health_ && is_alive()) {
        current_health_ = std::min(current_health_ + dt * 5.0f, max_health_);
        signal_health_changed_(current_health_);
    }
}

void HealthBehaviour::take_damage(float amount) {
    if (!is_alive()) return;

    current_health_ = std::max(0.0f, current_health_ - amount);
    signal_health_changed_(current_health_);

    if (!is_alive()) {
        signal_died_();
    }
}

void HealthBehaviour::heal(float amount) {
    if (!is_alive()) return;

    current_health_ = std::min(max_health_, current_health_ + amount);
    signal_health_changed_(current_health_);
}

} // namespace smlt
```

---

## 4. Attaching Behaviours to StageNodes

Behaviours are attached as **child nodes** of the visual nodes they augment. This is the standard pattern in Simulant:

```cpp
// Create the visual representation
auto player_actor = create_child<Actor>(mesh_id, DETAIL_LEVEL_NEAREST);
player_actor->set_name("Player");

// Attach behaviours as children
auto health = player_actor->create_child<HealthBehaviour>();
auto controller = player_actor->create_child<PlayerController>();
auto body = player_actor->create_child<DynamicBody>();
```

The resulting hierarchy looks like this:

```
Stage (root)
  `-- Actor "Player" (visual: mesh)
        |-- HealthBehaviour (game logic)
        |-- PlayerController (input + movement)
        `-- DynamicBody (physics simulation)
```

### Why Children, Not Components?

Simulant uses the scene graph hierarchy for behaviour composition rather than a separate component system. This has several advantages:

- **Transform inheritance**: Behaviour nodes automatically track the parent's position.
- **Automatic cleanup**: Destroying the parent destroys all behaviour children.
- **Uniform API**: Behaviours are `StageNode` instances, so they can be found by name, iterated, and receive update callbacks.
- **Signals**: Behaviours can fire `signal_destroyed()` and other lifecycle signals.

### Using Mixins for Flat Hierarchies

If you want behaviour without adding a child node to the hierarchy, use **mixins**. A mixin shares the parent's transform and receives the same update callbacks:

```cpp
auto camera = create_child<Camera3D>();

// Attach SmoothFollow as a mixin instead of a child
auto follow = camera->create_mixin<SmoothFollow>();

// The hierarchy stays flat:
//   Camera3D (with SmoothFollow mixin)
```

Mixins are ideal when the behaviour does not need to appear as a separate node in the scene graph. See [Stage Nodes - Mixins](../core-concepts/stage-nodes.md#10-mixins) for details.

---

## 5. The Behaviour Update Cycle

Every frame, the engine walks the scene graph and calls three update methods in a strict order:

### 5.1. `on_update(float dt)` -- Main Update

- **Called first** on all nodes before any other update phase.
- `dt` is the **variable** time since the last frame (in seconds).
- Use for: input handling, AI decisions, animation triggers, game logic.

```cpp
void on_update(float dt) override {
    // Move based on input
    Vec3 input = get_input_direction();
    transform->translate(input * speed_ * dt);

    // dt varies frame to frame:
    //   Frame 1: dt = 0.016 (60 fps)
    //   Frame 2: dt = 0.033 (30 fps)
    // Always multiply by dt for frame-rate-independent movement!
}
```

### 5.2. `on_fixed_update(float step)` -- Fixed Timestep

- Called at a **constant rate** regardless of frame rate.
- `step` is the fixed timestep value (typically 1/60 = 0.0167 seconds).
- Use for: physics calculations, deterministic simulation, network tick.

```cpp
void on_fixed_update(float step) override {
    // step is always the same value (e.g., 0.0167)
    // This ensures physics behaves identically regardless of frame rate
    apply_gravity(step);
    integrate_velocity(step);
}
```

### 5.3. `on_late_update(float dt)` -- Late Update

- **Called last**, after all `on_update()` calls have completed.
- Use for: camera follow, aim interpolation, anything that depends on other objects having moved.

```cpp
void on_late_update(float dt) override {
    // The player has already moved this frame, so we can follow them
    Vec3 target_pos = player->transform->get_translation();
    transform->set_translation(target_pos + offset_);
}
```

### Update Order Within a Frame

The engine processes the tree in this order:

```
For each node in tree (root to leaves):
    1. node->on_update(dt)
    2. For each mixin on node: mixin->on_update(dt)
    3. Recurse into children

For each node in tree:
    1. node->on_fixed_update(step)
    2. For each mixin: mixin->on_fixed_update(step)
    3. Recurse into children

For each node in tree:
    1. node->on_late_update(dt)
    2. For each mixin: mixin->on_late_update(dt)
    3. Recurse into children
```

### Participating in the Active Pipeline

Behaviours only receive update calls if their owning Stage is attached to an active render pipeline (Layer). You can check this:

```cpp
void on_update(float dt) override {
    if (!is_part_of_active_pipeline()) {
        return;  // Not being rendered, skip logic too
    }
    // Normal update logic...
}
```

---

## 6. Physics Behaviours

Simulant ships with four physics body types, all of which are `StageNode` subclasses that you attach to visual nodes.

### Inheritance Hierarchy

```
StageNode
  |-- PhysicsBody (abstract base)
  |     |-- StaticBody (immovable collider)
  |     `-- ReactiveBody (affected by forces)
  |           |-- DynamicBody (full physics simulation)
  |           `-- KinematicBody (script-controlled movement)
```

### 6.1. StaticBody

An immovable collider. Does not respond to forces but blocks other bodies.

```cpp
auto ground = create_child<Actor>(ground_mesh_id);
ground->scale_by(10, 0.1f, 10);
ground->move_to(0, -1, 0);

auto body = ground->create_child<StaticBody>();
body->add_box_collider(Vec3(5, 0.05f, 5), PhysicsMaterial());

// StaticBody properties (settable via params or directly)
body->set_friction(0.8f);
body->set_bounciness(0.1f);
```

**Use for:** Ground, walls, platforms, buildings -- anything that should not move.

### 6.2. DynamicBody

A fully simulated physics body. Responds to gravity, forces, and collisions.

```cpp
auto crate = create_child<Actor>(crate_mesh_id);
crate->move_to(0, 10, 0);

auto body = crate->create_child<DynamicBody>();
body->add_box_collider(Vec3(0.5f, 0.5f, 0.5f), PhysicsMaterial());
body->set_mass(1.0f);

// Optional: enable debug visualization
auto debug_vis = body->create_child<Debug>();
```

**Use for:** Boxes, balls, projectiles, debris -- anything that should move realistically.

### 6.3. KinematicBody

A body that moves under script control but still collides with other bodies. Physics does not move it; you do.

```cpp
class MovingPlatformBehaviour : public StageNode {
public:
    FindResult<KinematicBody> body = FindDescendent("Body", this);
    Vec3 start_pos_{0, 0, 0};
    Vec3 end_pos_{0, 5, 0};
    float t_ = 0;
    bool forward_ = true;

    MovingPlatformBehaviour(Scene* owner)
        : StageNode(owner, STAGE_NODE_TYPE_USER_BASE + 2, "moving_platform") {}

    void on_update(float dt) override {
        t_ += forward_ ? dt * 0.5f : -dt * 0.5f;
        if (t_ >= 1.0f) { t_ = 1.0f; forward_ = false; }
        if (t_ <= 0.0f) { t_ = 0.0f; forward_ = true; }

        Vec3 pos = glm::mix(start_pos_, end_pos_, t_);
        body->set_position(pos);
    }
};

// Usage:
auto platform = create_child<Actor>(platform_mesh_id);
auto kinematic = platform->create_child<KinematicBody>();
kinematic->add_box_collider(Vec3(2, 0.2f, 2), PhysicsMaterial());
auto mover = platform->create_child<MovingPlatformBehaviour>();
```

**Use for:** Moving platforms, elevators, doors, animated objects that should push dynamic bodies.

### 6.4. ReactiveBody

The base class for bodies that respond to forces. You typically use `DynamicBody` or `KinematicBody` directly, but `ReactiveBody` provides the common API:

```cpp
// All of these work on DynamicBody and KinematicBody
body->set_linear_velocity(Vec3(1, 0, 0));
body->set_angular_velocity(Vec3(0, 1, 0));
body->add_force(Vec3(0, 100, 0));
body->add_force_at_position(Vec3(0, 100, 0), application_point);
body->add_relative_force(Vec3(0, 0, 50));  // Force in body-local space
body->add_torque(Vec3(0, 10, 0));
body->add_relative_torque(Vec3(0, 10, 0));  // Torque in body-local space
body->add_impulse(Vec3(0, 50, 0));  // Instant velocity change
body->add_impulse_at_position(Vec3(0, 50, 0), hit_point);

// Acceleration force (ignores mass, useful for consistent movement)
body->add_acceleration_force(direction * acceleration);

// Damping
body->set_linear_damping(0.1f);
body->set_angular_damping(0.1f);

// Query state
Vec3 vel = body->linear_velocity();
Vec3 ang_vel = body->angular_velocity();
Vec3 forward = body->forward();
Vec3 right = body->right();
Vec3 up = body->up();
bool awake = body->is_awake();

// Lock specific rotation axes
body->lock_rotation(true, false, true);  // Only Y-axis rotation allowed

// Center of mass
body->set_center_of_mass(Vec3(0, -0.2f, 0));
Vec3 com = body->center_of_mass();
Vec3 abs_com = body->absolute_center_of_mass();
```

### 6.5. Collider Shapes

All physics bodies support multiple collider shapes (fixtures):

```cpp
// Box collider
body->add_box_collider(
    Vec3(1, 1, 1),           // Size (half-extents)
    PhysicsMaterial(),        // Material properties
    0,                        // Kind (collision category)
    Vec3(0, 0.5f, 0),        // Offset from body center
    Quaternion()              // Rotation
);

// Sphere collider
body->add_sphere_collider(
    0.5f,                     // Diameter
    PhysicsMaterial(),
    0,
    Vec3(0, 0, 0)            // Offset
);

// Capsule collider (great for characters)
body->add_capsule_collider(
    Vec3(0, -0.5f, 0),       // Bottom point
    Vec3(0, 0.5f, 0),        // Top point
    0.3f,                     // Diameter
    PhysicsMaterial(),
    0
);

// Triangle collider (for flat surfaces)
body->add_triangle_collider(
    Vec3(-1, 0, -1),         // Vertex 1
    Vec3(1, 0, -1),          // Vertex 2
    Vec3(0, 0, 1),           // Vertex 3
    PhysicsMaterial(),
    0
);
```

### 6.6. Collision Detection

React to collisions using signals or the `CollisionListener` interface:

#### Using Signals

```cpp
class DamageOnHit : public StageNode {
public:
    FindResult<DynamicBody> body = FindDescendent("Body", this);
    FindResult<HealthBehaviour> health = FindAncestor("Health", this);

    S_DEFINE_STAGE_NODE_META(STAGE_NODE_TYPE_USER_BASE + 3, "damage_on_hit");

    DamageOnHit(Scene* owner)
        : StageNode(owner, STAGE_NODE_TYPE_USER_BASE + 3) {}

protected:
    bool on_create(Params params) override {
        body->signal_collision_enter().connect([this](const Collision& c) {
            // c.other_body is the body we collided with
            // c.contact_points contains detailed collision data
            float damage = 10.0f;
            if (health) {
                health->take_damage(damage);
            }
        });
        return true;
    }
};
```

#### Using CollisionListener

For more complex collision handling with enter/stay/exit and trigger events:

```cpp
class MyCollisionListener : public CollisionListener {
public:
    void on_collision_enter(const Collision& collision) override {
        S_INFO("Hit: {}", collision.other_collider_name);
    }

    void on_collision_stay() override {
        // Called every fixed update while in contact
    }

    void on_collision_exit(const Collision& collision) override {
        S_INFO("Stopped hitting: {}", collision.other_collider_name);
    }

    void on_trigger_enter() override {
        S_INFO("Entered trigger zone");
    }

    void on_trigger_stay() override {
        // Called every fixed update while inside trigger
    }

    void on_trigger_exit() override {
        S_INFO("Left trigger zone");
    }
};

// Register the listener:
body->register_collision_listener(&my_listener);

// Unregister when done (or let the destructor handle it):
// body->unregister_collision_listener(&my_listener);
```

### 6.7. Sensors (Trigger Zones)

Fixtures can act as sensors -- they detect overlap without physically blocking objects:

```cpp
// Create a trigger zone using a sensor
auto trigger = create_child<Actor>();
trigger->set_name("PickupZone");

auto body = trigger->create_child<DynamicBody>();
body->add_sphere_collider(2.0f, PhysicsMaterial());

// Make it a sensor by accessing the underlying physics system
// Sensors detect overlap without physical collision response
body->signal_collision_enter().connect([this](const Collision& c) {
    S_INFO("Object entered trigger zone");
    on_pickup(c.other_body);
});
```

For a more complete trigger zone pattern, see [Common Behaviour Patterns](#10-common-behaviour-patterns) below.

### 6.8. Joints

Connect two physics bodies with constraints:

```cpp
// Sphere joint: body_b can move within a sphere around body_a's anchor
auto joint = body_a->create_sphere_joint(
    body_b,                              // Other body
    Vec3(0, 1, 0),                       // Anchor on body_a (local)
    Vec3(0, -1, 0)                       // Anchor on body_b (local)
);

// The joint is managed by the physics service and destroyed with the bodies
```

---

## 7. Physics Service Management

Physics simulation in Simulant is managed by the `PhysicsService`, which is registered as a service on the application. You do not create physics worlds manually -- the service handles registration, stepping, and cleanup of all physics bodies.

### Gravity

```cpp
// Set gravity during scene initialisation
auto physics_service = application->find_service<PhysicsService>();
physics_service->set_gravity(Vec3(0, -9.81f, 0));  // Earth gravity

// Zero gravity for space games
physics_service->set_gravity(Vec3(0, 0, 0));
```

### Raycasting

Query the physics world with rays:

```cpp
auto physics_service = application->find_service<PhysicsService>();

Vec3 ray_start = camera_pos;
Vec3 ray_dir = camera_forward;

auto result = physics_service->ray_cast(ray_start, ray_dir, 100.0f);

if (result.has_value()) {
    S_INFO("Hit at distance: {}", result->distance);
    S_INFO("Hit normal: {}", result->normal);
    S_INFO("Impact point: {}", result->impact_point);
    S_INFO("Hit body: {}", result->other_body);
}
```

### Custom Contact Filter

Override collision behaviour by setting a custom contact filter:

```cpp
class GameContactFilter : public ContactFilter {
public:
    bool should_collide(const Fixture* lhs, const Fixture* rhs) const override {
        // Prevent player-player collision
        if (lhs->kind() == 1 && rhs->kind() == 1) return false;
        return true;
    }

    bool should_respond(const Fixture* lhs, const Fixture* rhs) const override {
        // Bullets pass through each other but still register hits
        if (lhs->kind() == 2 && rhs->kind() == 2) return false;
        return true;
    }
};

GameContactFilter filter;
physics_service->set_contact_filter(&filter);
```

### Debug Visualization

```cpp
auto debug_node = create_child<Debug>();
physics_service->set_debug(debug_node);
```

### Pausing Physics

Physics steps happen through the service's `on_fixed_update()`. To pause physics, you can disable bodies or stop the service from processing. Bodies that are not part of an active pipeline will not be updated.

---

## 8. Finding Other Behaviours and Nodes

Behaviours rarely exist in isolation. They need references to other nodes and behaviours. Simulant provides several mechanisms for this.

### 8.1. FindResult<T> Helpers

`FindResult<T>` provides **lazy, cached lookups** that automatically invalidate when the target is destroyed. Declare them as class members:

```cpp
class PlayerController : public StageNode {
public:
    // Find a direct child named "Body" of type DynamicBody
    FindResult<DynamicBody> body = FindDescendent("Body", this);

    // Find a child of a specific type (immediate children only)
    FindResult<Debug> debug = FindChild<Debug>(this);

    // Find an ancestor by name
    FindResult<Actor> player_visual = FindAncestor("Player", this);

    // Find a mixin on the parent node
    FindResult<SmoothFollow> follow = FindMixin<SmoothFollow>(this);

protected:
    void on_update(float dt) override {
        // First access performs the search and caches the result
        if (body) {
            body->add_force(Vec3(0, 100, 0));
        }

        // Subsequent accesses use the cached result
        body->add_force(Vec3(0, 50, 0));

        // Check if cache is valid without triggering a search
        if (body.is_cached()) {
            S_DEBUG("Body reference is cached");
        }
    }
};
```

### 8.2. How FindResult Works

1. **Lazy evaluation**: No search happens until you first access the result (e.g., `body->` or `if (body)`).
2. **Caching**: Once found, the pointer is cached for fast subsequent access.
3. **Auto-invalidation**: If the found node is destroyed, the cache clears automatically via `signal_destroyed()`.
4. **Re-search**: The next access after invalidation performs a fresh search.

### 8.3. Available Finders

| Finder | Searches | Example |
|--------|----------|---------|
| `FindDescendent(name, node)` | Entire subtree for a named node | `FindDescendent("Wheel", this)` |
| `FindDescendentByID(id, node)` | Entire subtree by unique ID | `FindDescendentByID(some_id, this)` |
| `FindChild<T>(node)` | Immediate children by type | `FindChild<Debug>(this)` |
| `FindAncestor(name, node)` | Up the tree for a named node | `FindAncestor("Player", this)` |
| `FindMixin<T>(node)` | Mixins on a node by type | `FindMixin<SmoothFollow>(this)` |

### 8.4. Manual Lookup

For one-off searches (not cached):

```cpp
// Find by name anywhere in the subtree
StageNode* node = stage->find_descendent_with_name("Enemy_01");

// Find by type
std::vector<StageNode*> cameras = stage->find_descendents_by_types(
    {STAGE_NODE_TYPE_CAMERA3D}
);
```

---

## 9. Communication Between Behaviours

Behaviours need to communicate with each other. Simulant offers several patterns, from loose to tight coupling.

### 9.1. Signals (Recommended)

The most decoupled approach. The sender defines signals; listeners connect without the sender knowing who they are:

```cpp
// HealthBehaviour defines signals
DEFINE_SIGNAL(sig::signal<void(float)>, signal_health_changed);
DEFINE_SIGNAL(sig::signal<void()>, signal_died);

// UIHealthBar listens
class UIHealthBar : public StageNode {
public:
    FindResult<HealthBehaviour> health = FindAncestor("Health", this);

protected:
    bool on_create(Params params) override {
        health->signal_health_changed().connect([this](float hp) {
            update_bar(hp);
        });
        health->signal_died().connect([this]() {
            show_death_animation();
        });
        return true;
    }
};
```

### 9.2. Direct References via FindResult

When two behaviours have a fixed relationship (parent-child, ancestor-descendant), use `FindResult`:

```cpp
class WeaponController : public StageNode {
public:
    // The controller finds its sibling HealthBehaviour on the same parent
    FindResult<HealthBehaviour> health = FindDescendent("Health", this);

    void fire() {
        if (health && health->current_health() > 0) {
            spawn_projectile();
        }
    }
};
```

### 9.3. Shared State via Parent

Siblings can share data through a common parent behaviour:

```cpp
class TankController : public StageNode {
public:
    FindResult<DynamicBody> body = FindDescendent("Body", this);
    FindResult<TurretController> turret = FindDescendent("Turret", this);

    void set_target(Vec3 target) {
        target_ = target;
        // Both the body (movement) and turret (aiming) react to the same target
    }

private:
    Vec3 target_;
};
```

### 9.4. Scene-Wide Events via Global Signals

For communication between unrelated behaviours, connect to scene or application signals:

```cpp
// Any behaviour can listen to scene signals:
scene->signal_stage_node_created().connect(
    [this](StageNode* node, StageNodeType type) {
        // React to new objects appearing
    }
);

// Or application-level signals:
application->signal_shutdown().connect([this]() {
    save_state();
});
```

---

## 10. Common Behaviour Patterns

### 10.1. Player Controller

A controller that reads input and moves a physics body:

```cpp
class PlayerController : public StageNode {
public:
    FindResult<DynamicBody> body = FindDescendent("Body", this);
    FindResult<Actor> visual = FindAncestor("Player", this);

    S_DEFINE_STAGE_NODE_META(STAGE_NODE_TYPE_USER_BASE + 10, "player_controller");

    PlayerController(Scene* owner)
        : StageNode(owner, STAGE_NODE_TYPE_USER_BASE + 10) {}

protected:
    bool on_create(Params params) override {
        // Connect to death signal for respawn logic
        // (assuming HealthBehaviour is a sibling)
        return true;
    }

    void on_update(float dt) override {
        if (!body) return;

        // Read input from keyboard/gamepad
        Vec3 input(0, 0, 0);

        if (application->keyboard->is_key_down(SDLK_w)) input.z += 1;
        if (application->keyboard->is_key_down(SDLK_s)) input.z -= 1;
        if (application->keyboard->is_key_down(SDLK_a)) input.x -= 1;
        if (application->keyboard->is_key_down(SDLK_d)) input.x += 1;

        if (input.length() > 0) {
            input = input.normalized();

            // Acceleration-based movement (frame-rate independent)
            body->add_acceleration_force(input * move_accel_);

            // Jump
            if (application->keyboard->is_key_down(SDLK_SPACE) && body->is_awake()) {
                body->add_impulse(Vec3(0, jump_impulse_, 0));
            }
        }

        // Rotate visual to face movement direction
        if (input.length() > 0.1f && visual) {
            Quaternion target_rot = Quaternion::look_at(input, Vec3(0, 1, 0));
            visual->transform->set_rotation(target_rot);
        }
    }

private:
    float move_accel_ = 50.0f;
    float jump_impulse_ = 50.0f;
};
```

Usage:

```cpp
auto player = create_child<Actor>(player_mesh_id);
player->set_name("Player");

auto body = player->create_child<DynamicBody>();
body->add_capsule_collider(Vec3(0, -0.5f, 0), Vec3(0, 0.5f, 0), 0.4f, PhysicsMaterial());
body->set_mass(80.0f);

auto controller = player->create_child<PlayerController>();
```

### 10.2. Enemy AI

A simple patrol and chase AI:

```cpp
enum class EnemyState { PATROL, CHASE, ATTACK };

class EnemyAI : public StageNode {
public:
    FindResult<DynamicBody> body = FindDescendent("Body", this);
    FindResult<Actor> visual = FindAncestor("Enemy", this);

    S_DEFINE_STAGE_NODE_META(STAGE_NODE_TYPE_USER_BASE + 11, "enemy_ai");

    EnemyAI(Scene* owner)
        : StageNode(owner, STAGE_NODE_TYPE_USER_BASE + 11) {}

protected:
    void on_update(float dt) override {
        if (!body || !player_) return;

        float dist_to_player = (player_->transform->get_translation()
            - transform->get_translation()).length();

        switch (state_) {
            case EnemyState::PATROL:
                patrol(dt);
                if (dist_to_player < detect_range_) {
                    state_ = EnemyState::CHASE;
                }
                break;

            case EnemyState::CHASE:
                chase(dt);
                if (dist_to_player < attack_range_) {
                    state_ = EnemyState::ATTACK;
                } else if (dist_to_player > lose_range_) {
                    state_ = EnemyState::PATROL;
                }
                break;

            case EnemyState::ATTACK:
                attack(dt);
                if (dist_to_player > attack_range_ * 1.5f) {
                    state_ = EnemyState::CHASE;
                }
                break;
        }
    }

private:
    void patrol(float dt) {
        // Move to next waypoint
        if (current_waypoint_ >= waypoints_.size()) {
            current_waypoint_ = 0;
        }

        Vec3 target = waypoints_[current_waypoint_];
        Vec3 dir = (target - transform->get_translation()).normalized();
        body->add_acceleration_force(dir * patrol_speed_);

        if ((target - transform->get_translation()).length() < 1.0f) {
            current_waypoint_++;
        }
    }

    void chase(float dt) {
        Vec3 to_player = player_->transform->get_translation()
                       - transform->get_translation();
        Vec3 dir = to_player.normalized();
        body->add_acceleration_force(dir * chase_speed_);
    }

    void attack(float dt) {
        attack_timer_ += dt;
        if (attack_timer_ >= attack_cooldown_) {
            attack_timer_ = 0;
            perform_attack();
        }
    }

    void perform_attack() {
        // Launch a projectile or apply damage
    }

    EnemyState state_ = EnemyState::PATROL;
    StageNode* player_ = nullptr;
    std::vector<Vec3> waypoints_;
    size_t current_waypoint_ = 0;
    float detect_range_ = 15.0f;
    float attack_range_ = 3.0f;
    float lose_range_ = 25.0f;
    float patrol_speed_ = 10.0f;
    float chase_speed_ = 25.0f;
    float attack_cooldown_ = 1.0f;
    float attack_timer_ = 0;
};
```

### 10.3. Trigger Zones

A zone that fires events when objects enter, stay, or leave:

```cpp
class TriggerZone : public StageNode {
public:
    FindResult<DynamicBody> body = FindDescendent("Body", this);

    S_DEFINE_STAGE_NODE_META(STAGE_NODE_TYPE_USER_BASE + 12, "trigger_zone");

    TriggerZone(Scene* owner)
        : StageNode(owner, STAGE_NODE_TYPE_USER_BASE + 12) {}

    DEFINE_SIGNAL(sig::signal<void(PhysicsBody*)>, signal_entity_entered);
    DEFINE_SIGNAL(sig::signal<void(PhysicsBody*)>, signal_entity_stayed);
    DEFINE_SIGNAL(sig::signal<void(PhysicsBody*)>, signal_entity_exited);

protected:
    bool on_create(Params params) override {
        body->signal_collision_enter().connect([this](const Collision& c) {
            entered_bodies_.insert(c.other_body);
            signal_entity_entered_(c.other_body);
        });

        body->signal_collision_exit().connect([this](const Collision& c) {
            entered_bodies_.erase(c.other_body);
            signal_entity_exited_(c.other_body);
        });

        return true;
    }

    void on_fixed_update(float step) override {
        for (auto* body_ptr : entered_bodies_) {
            signal_entity_stayed_(body_ptr);
        }
    }

private:
    std::set<PhysicsBody*> entered_bodies_;
};
```

Usage as a pickup zone:

```cpp
auto pickup = create_child<Actor>();
pickup->set_name("HealthPickup");
pickup->move_to(5, 1, 3);

// Visual indicator (transparent sphere)
auto sphere_mesh = assets->new_mesh_from_procedural_sphere(2.0f);
pickup->set_mesh(sphere_mesh->id());

// Physics body with large sensor
auto trigger_body = pickup->create_child<DynamicBody>();
trigger_body->add_sphere_collider(2.0f, PhysicsMaterial());

// Trigger logic
auto trigger = pickup->create_child<TriggerZone>();
trigger->signal_entity_entered().connect([this](PhysicsBody* other) {
    auto* actor = other->parent.get();
    if (actor && actor->name() == "Player") {
        auto* health = actor->find_descendent_with_name("Health");
        if (health) {
            // Heal the player
        }
        // Remove the pickup
        this->destroy();
    }
});
```

### 10.4. Object Pooling

Reuse objects instead of creating and destroying them repeatedly:

```cpp
class ProjectilePool : public StageNode {
public:
    S_DEFINE_STAGE_NODE_META(STAGE_NODE_TYPE_USER_BASE + 13, "projectile_pool");

    ProjectilePool(Scene* owner)
        : StageNode(owner, STAGE_NODE_TYPE_USER_BASE + 13) {}

    // Initialise the pool with a number of projectiles
    void initialise(size_t count, MeshID mesh_id) {
        mesh_id_ = mesh_id;
        for (size_t i = 0; i < count; ++i) {
            auto proj = create_child<Actor>(mesh_id);
            proj->set_name("Projectile_" + std::to_string(i));
            proj->set_visible(false);
            proj->set_parent(this);
            available_.push_back(proj);
        }
    }

    // Get an available projectile (returns nullptr if pool exhausted)
    Actor* acquire() {
        if (available_.empty()) {
            S_WARN("Projectile pool exhausted");
            return nullptr;
        }

        Actor* proj = available_.front();
        available_.pop_front();
        proj->set_visible(true);
        in_use_.insert(proj);
        return proj;
    }

    // Return a projectile to the pool
    void release(Actor* proj) {
        if (!proj || in_use_.find(proj) == in_use_.end()) return;

        proj->set_visible(false);
        proj->move_to(0, -1000, 0);  // Hide off-screen
        in_use_.erase(proj);
        available_.push_back(proj);
    }

    // Release all at once
    void release_all() {
        for (auto* proj : in_use_) {
            release(proj);
        }
    }

    size_t available_count() const { return available_.size(); }
    size_t in_use_count() const { return in_use_.size(); }

private:
    MeshID mesh_id_;
    std::list<Actor*> available_;
    std::set<Actor*> in_use_;
};
```

Usage:

```cpp
auto pool = create_child<ProjectilePool>();
pool->initialise(50, projectile_mesh_id);

// Later, when firing:
Actor* projectile = pool->acquire();
if (projectile) {
    projectile->move_to(gun_position);
    // Attach a physics body or movement behaviour
}
```

---

## 11. Behaviour Best Practices

### 11.1. Single Responsibility

Each behaviour should do one thing well.

```cpp
// GOOD: Separate concerns
auto body = actor->create_child<DynamicBody>();       // Physics
auto health = actor->create_child<HealthBehaviour>(); // Health management
auto ai = actor->create_child<EnemyAI>();             // Decision making
auto anim = actor->create_child<AnimationController>(); // Animation

// BAD: One giant behaviour that does everything
class GodEnemyBehaviour : public StageNode {
    // Physics, health, AI, animation, sound, particles, UI updates...
};
```

### 11.2. Use FindResult for Stable References

Avoid repeated name-based lookups in `on_update()`:

```cpp
// GOOD: Cached, lazy lookup
FindResult<DynamicBody> body = FindDescendent("Body", this);

void on_update(float dt) override {
    body->add_force(force);  // Cached after first call
}

// BAD: Repeated string search every frame
void on_update(float dt) override {
    auto* body = (DynamicBody*) find_descendent_with_name("Body");
    // O(n) string search every frame!
}
```

### 11.3. Always Multiply by dt

Movement and time-based calculations must use `dt` for frame-rate independence:

```cpp
// GOOD: Frame-rate independent
void on_update(float dt) override {
    transform->translate(direction * speed_ * dt);
}

// BAD: Speed depends on frame rate
void on_update(float dt) override {
    transform->translate(direction * speed_);  // Moves faster at higher FPS!
}
```

### 11.4. Check Pointers Before Use

`FindResult` can be null if the target was destroyed or never existed:

```cpp
void on_update(float dt) override {
    if (!body) {
        // Body was destroyed or not found
        return;
    }
    body->add_force(force);
}
```

### 11.5. Use Signals for Events, FindResult for State

```cpp
// GOOD: Signal for one-time events
health->signal_died().connect([this]() {
    spawn_death_particles();
});

// GOOD: FindResult for ongoing state
FindResult<HealthBehaviour> health = FindDescendent("Health", this);
void on_update(float dt) override {
    if (health && health->current_health() < 30.0f) {
        play_low_health_effect();
    }
}
```

### 11.6. Clean Up on Destroy

Disconnect signals and release resources:

```cpp
bool on_destroy() override {
    // Connections to signals on other objects are automatically cleaned
    // when this node is destroyed, but explicit cleanup is clearer:

    if (health_) {
        health_conn_.disconnect();
    }

    return StageNode::on_destroy();
}
```

### 11.7. Register Types Early

Register all custom behaviour types in your scene's `on_load()` before creating any nodes:

```cpp
void on_load() override {
    // Register first
    register_stage_node<PlayerController>();
    register_stage_node<HealthBehaviour>();
    register_stage_node<EnemyAI>();

    // Then create
    create_player();
    create_enemies();
}
```

---

## 12. Performance Considerations

### 12.1. Avoid Deep Hierarchies

Each level of the scene graph adds overhead to tree traversal during updates. Keep hierarchies shallow where possible:

```
// GOOD: Flat hierarchy with mixins
Actor "Player" (with Controller mixin, Health mixin, AnimationController mixin)

// BAD: Deep hierarchy
Actor "Player"
  |-- Controller
  |     |-- InputHandler
  |     `-- MovementLogic
  |-- Health
  |     `-- Regeneration
  `-- AnimationController
        `-- StateMachine
```

### 12.2. Cache FindResult Lookups

`FindResult` caches its result after the first access. Always use member declarations rather than local variables:

```cpp
class MyBehaviour : public StageNode {
    // GOOD: Member-level FindResult (cached)
    FindResult<DynamicBody> body = FindDescendent("Body", this);

    void on_update(float dt) override {
        // GOOD: Uses cached result
        body->add_force(force);
    }
};
```

### 12.3. Minimise Signal Connections in Hot Paths

Each signal connection is a function call during emission. For events that fire every frame with many listeners, consider direct method calls:

```cpp
// For high-frequency events, direct calls can be faster:
class PhysicsUpdater : public StageNode {
public:
    void update_body(DynamicBody* body, float dt) {
        body->add_force(force);
    }
};

// Rather than:
// signal_physics_update_.emit(body, dt);  // 50 listeners = 50 calls
```

### 12.4. Use Appropriate Physics Colliders

Simpler colliders are faster to simulate:

```cpp
// FASTEST: Sphere
body->add_sphere_collider(radius, PhysicsMaterial());

// FAST: Box
body->add_box_collider(size, PhysicsMaterial());

// MODERATE: Capsule
body->add_capsule_collider(bottom, top, diameter, PhysicsMaterial());

// SLOWEST: Avoid for dynamic objects
// Triangle mesh colliders are expensive
```

### 12.5. Disable Unnecessary Updates

Behaviours that are not part of an active pipeline do not receive updates. If you have logic-only nodes that do not need per-frame updates, consider using signals or coroutines instead.

### 12.6. Object Pooling for Frequent Spawn/Destroy

Creating and destroying nodes has overhead. For objects that spawn and die frequently (bullets, particles, enemies), use object pooling (see [Object Pooling pattern](#104-object-pooling) above).

### 12.7. Batch Similar Updates

If you have many behaviours doing similar work, consider processing them in a single behaviour rather than spreading across many nodes:

```cpp
// Instead of 100 enemy behaviours each checking distance to player:
class EnemyManager : public StageNode {
public:
    FindResult<Actor> player = FindDescendent("Player", this);

    void on_update(float dt) override {
        Vec3 player_pos = player->transform->get_translation();

        for (auto& child : each_descendent()) {
            if (auto* enemy = dynamic_cast<EnemyBehaviour*>(&child)) {
                float dist = (enemy->transform->get_translation() - player_pos).length();
                enemy->set_player_distance(dist);
            }
        }
    }
};
```

---

## 13. Complete Examples

### 13.1. Complete Third-Person Character Controller

```cpp
#pragma once
#include <simulant/simulant.h>

namespace smlt {

class ThirdPersonController : public StageNode {
public:
    S_DEFINE_STAGE_NODE_META(STAGE_NODE_TYPE_USER_BASE + 20, "third_person_controller");

    FindResult<DynamicBody> body = FindDescendent("Body", this);
    FindResult<Actor> visual = FindAncestor("Player", this);
    FindResult<Camera3D> camera = FindDescendent("Camera", this);
    FindResult<AnimationController> anim = FindDescendent("Animation", this);

    ThirdPersonController(Scene* owner)
        : StageNode(owner, STAGE_NODE_TYPE_USER_BASE + 20) {}

protected:
    bool on_create(Params params) override {
        camera_offset_ = Vec3(0, 5, -8);
        is_grounded_ = false;
        return true;
    }

    void on_update(float dt) override {
        if (!body || !visual) return;

        // Get camera-relative input
        Vec3 forward = camera->transform->forward();
        forward.y = 0;
        forward = forward.normalized();

        Vec3 right = camera->transform->right();
        right.y = 0;
        right = right.normalized();

        Vec3 input(0, 0, 0);
        if (application->keyboard->is_key_down(SDLK_w)) input += forward;
        if (application->keyboard->is_key_down(SDLK_s)) input -= forward;
        if (application->keyboard->is_key_down(SDLK_d)) input += right;
        if (application->keyboard->is_key_down(SDLK_a)) input -= right;

        // Movement
        if (input.length() > 0.01f) {
            input = input.normalized();
            body->add_acceleration_force(input * 80.0f);

            // Rotate visual to face movement direction
            float angle = atan2(input.x, input.z);
            Quaternion target = Quaternion::from_euler(0, angle, 0);
            visual->transform->set_rotation(target);

            // Run animation
            if (anim) {
                anim->play("run", true);
            }
        } else {
            // Idle animation
            if (anim) {
                anim->play("idle", true);
            }
        }

        // Jump
        if (application->keyboard->is_key_down(SDLK_SPACE) && is_grounded_) {
            body->add_impulse(Vec3(0, 80.0f, 0));
            is_grounded_ = false;
        }

        // Dampen horizontal movement for control
        Vec3 vel = body->linear_velocity();
        Vec3 damping = Vec3(-vel.x * 2.0f, 0, -vel.z * 2.0f);
        body->add_acceleration_force(damping);
    }

    void on_late_update(float dt) override {
        // Camera follows behind the player
        if (camera && visual) {
            Vec3 player_pos = visual->transform->get_translation();
            Vec3 cam_pos = player_pos + camera_offset_;
            camera->transform->set_translation(cam_pos);
            camera->transform->look_at(player_pos, Vec3(0, 1, 0));
        }
    }

private:
    Vec3 camera_offset_;
    bool is_grounded_;
};

} // namespace smlt
```

### 13.2. Complete Scene with Physics

```cpp
#pragma once
#include <simulant/simulant.h>

namespace smlt {

class PhysicsDemoScene : public Scene<PhysicsDemoScene> {
protected:
    void on_load() override {
        // Register custom behaviours
        register_stage_node<PlayerController>();
        register_stage_node<HealthBehaviour>();
        register_stage_node<BouncingBall>();

        // Set up gravity
        auto physics = application->find_service<PhysicsService>();
        physics->set_gravity(Vec3(0, -9.81f, 0));

        create_ground();
        create_walls();
        create_player();
        create_bouncing_balls();

        // Set up camera
        setup_camera();
    }

private:
    void create_ground() {
        auto ground = create_child<Actor>(
            assets->new_mesh_from_procedural_cube()->id()
        );
        ground->set_name("Ground");
        ground->scale_by(20, 0.2f, 20);
        ground->move_to(0, -0.1f, 0);

        auto body = ground->create_child<StaticBody>();
        body->add_box_collider(Vec3(10, 0.1f, 10), PhysicsMaterial());
    }

    void create_walls() {
        Vec3 wall_positions[] = {
            {10, 1, 0}, {-10, 1, 0}, {0, 1, 10}, {0, 1, -10}
        };
        Vec3 wall_scales[] = {
            {0.2f, 2, 20}, {0.2f, 2, 20}, {20, 2, 0.2f}, {20, 2, 0.2f}
        };

        for (int i = 0; i < 4; ++i) {
            auto wall = create_child<Actor>(
                assets->new_mesh_from_procedural_cube()->id()
            );
            wall->scale_by(wall_scales[i].x, wall_scales[i].y, wall_scales[i].z);
            wall->move_to(wall_positions[i]);

            auto body = wall->create_child<StaticBody>();
            body->add_box_collider(
                Vec3(wall_scales[i].x * 5, wall_scales[i].y, wall_scales[i].z * 5),
                PhysicsMaterial()
            );
        }
    }

    void create_player() {
        auto player = create_child<Actor>(
            assets->new_mesh_from_procedural_cube()->id()
        );
        player->set_name("Player");
        player->move_to(0, 2, 0);
        player->scale_by(0.5f, 1.0f, 0.5f);

        // Physics body (capsule)
        auto body = player->create_child<DynamicBody>();
        body->add_capsule_collider(
            Vec3(0, -0.4f, 0), Vec3(0, 0.4f, 0), 0.4f, PhysicsMaterial()
        );
        body->set_mass(80.0f);
        body->set_linear_damping(0.5f);

        // Health
        auto health = player->create_child<HealthBehaviour>();

        // Controller
        auto controller = player->create_child<PlayerController>();
    }

    void create_bouncing_balls() {
        for (int i = 0; i < 10; ++i) {
            auto ball = create_child<Actor>(
                assets->new_mesh_from_procedural_sphere(0.3f)->id()
            );
            ball->move_to(
                (rand() % 10) - 5,
                5 + (rand() % 5),
                (rand() % 10) - 5
            );

            auto body = ball->create_child<DynamicBody>();
            body->add_sphere_collider(0.6f, PhysicsMaterial());
            body->set_mass(1.0f);
            body->set_bounciness(0.8f);

            auto bounce = ball->create_child<BouncingBall>();
        }
    }

    void setup_camera() {
        auto camera = create_child<Camera3D>();
        camera->move_to(0, 8, -15);
        camera->transform->look_at(Vec3(0, 2, 0), Vec3(0, 1, 0));

        // Set up render pipeline
        auto pipeline = window->compositor->create_layer(scene(), camera);
        pipeline->activate();
    }
};

} // namespace smlt
```

### 13.3. Complete Bouncing Ball Behaviour

```cpp
#pragma once
#include <simulant/simulant.h>

namespace smlt {

class BouncingBall : public StageNode {
public:
    S_DEFINE_STAGE_NODE_META(STAGE_NODE_TYPE_USER_BASE + 21, "bouncing_ball");
    FindResult<DynamicBody> body = FindDescendent("Body", this);
    FindResult<Actor> visual = FindAncestor("Ball", this);

    BouncingBall(Scene* owner)
        : StageNode(owner, STAGE_NODE_TYPE_USER_BASE + 21) {}

protected:
    bool on_create(Params params) override {
        bounce_count_ = 0;

        // Listen for collisions
        if (body) {
            body->signal_collision_enter().connect([this](const Collision& c) {
                bounce_count_++;
                S_DEBUG("Ball bounced {} times", bounce_count_);

                // Change colour on bounce
                if (visual) {
                    float hue = fmod(bounce_count_ * 0.1f, 1.0f);
                    // Could change material colour here
                }

                // Stop tracking after many bounces to save performance
                if (bounce_count_ > 50) {
                    this->destroy();
                }
            });
        }

        return true;
    }

    void on_update(float dt) override {
        if (!body) return;

        // Auto-destroy if the ball has stopped moving and is at rest
        if (bounce_count_ > 5) {
            Vec3 vel = body->linear_velocity();
            if (vel.length() < 0.1f && !body->is_awake()) {
                // Ball has come to rest
                S_DEBUG("Ball came to rest after {} bounces", bounce_count_);
            }
        }
    }

private:
    int bounce_count_ = 0;
};

} // namespace smlt
```

---

## Summary

| Topic | Key Takeaway |
|-------|-------------|
| **What behaviours are** | `StageNode` subclasses that implement game logic as children of visual nodes. |
| **Lifecycle** | `on_create` -> `on_update` -> `on_fixed_update` -> `on_late_update` -> `on_destroy`. |
| **Custom behaviours** | Inherit `StageNode`, define metadata with `S_DEFINE_STAGE_NODE_META`, register with `register_stage_node<T>()`. |
| **Attaching** | Use `create_child<BehaviourType>()` on the visual node. |
| **Update cycle** | `on_update` (variable dt), `on_fixed_update` (constant step), `on_late_update` (after all updates). |
| **Physics** | `StaticBody`, `DynamicBody`, `KinematicBody`, `ReactiveBody` -- all `StageNode` subclasses. |
| **Finding nodes** | Use `FindResult<T>` members for cached, auto-invalidating lookups. |
| **Communication** | Signals for events, `FindResult` for state, scene signals for global events. |
| **Performance** | Keep hierarchies shallow, cache lookups, use simple colliders, pool frequently created objects. |
