# Rigid Bodies

Rigid bodies are the core of Simulant's physics simulation. They represent physical objects that move, collide, and interact within the physics world. This guide covers the different body types, their properties, and how to use them effectively.

**Related documentation:** [Physics Overview](overview.md), [Colliders](colliders.md), [Joints](joints.md).

---

## 1. Body Types

Simulant provides three primary rigid body types, each designed for a specific use case:

| Body Type | Class | Affected by Forces | Can Move | Pushes Others |
|-----------|-------|-------------------|----------|---------------|
| Dynamic | `DynamicBody` | Yes | Yes (physics-driven) | Yes |
| Static | `StaticBody` | No | No | No |
| Kinematic | `KinematicBody` | No | Yes (script-driven) | Yes |

All body types are subclasses of `PhysicsBody`, which provides shared functionality like position, orientation, collider management, and collision signals.

---

## 2. DynamicBody

A `DynamicBody` is a fully simulated physics object. It responds to gravity, forces, impulses, and collisions with other bodies. This is the most common body type for objects that should behave physically.

### Creating a DynamicBody

```cpp
class CrateBehaviour : public StageNode {
public:
    void on_load() override {
        // Create the visual
        auto actor = create_child<Actor>();
        auto mesh = assets->new_mesh_from_procedural_cube();
        actor->set_mesh(mesh->id());
        actor->scale_by(1, 1, 1);

        // Create the physics body
        auto body = actor->create_child<DynamicBody>();

        // Add a box collider with material properties
        PhysicsMaterial material = PhysicsMaterial::wood();
        body->add_box_collider(Vec3(1, 1, 1), material);

        // Set mass (otherwise uses density-based calculation)
        body->set_mass(2.0f);
    }
};
```

### Applying Forces

Dynamic bodies respond to various types of forces:

```cpp
auto body = /* your DynamicBody */;

// Apply a continuous force at the center of mass
body->add_force(Vec3(0, 100, 0));

// Apply force at a specific point (causes rotation)
body->add_force_at_position(Vec3(0, 100, 0), Vec3(0.5f, 0, 0));

// Apply a force in the body's local coordinate space
body->add_relative_force(Vec3(0, 0, 50));  // "Forward" in local space

// Apply an impulse (instantaneous velocity change)
body->add_impulse(Vec3(0, 200, 0));

// Apply impulse at a specific point
body->add_impulse_at_position(Vec3(0, 200, 0), Vec3(0.5f, 0, 0));

// Apply torque (rotational force)
body->add_torque(Vec3(0, 10, 0));

// Apply torque in local space
body->add_relative_torque(Vec3(0, 10, 0));
```

### Acceleration Force

When you want to apply a force that ignores mass (direct acceleration):

```cpp
// Equivalent to add_force(acceleration * mass)
body->add_acceleration_force(Vec3(0, 9.81f, 0));

// At a specific point
body->add_acceleration_force_at_position(Vec3(0, 9.81f, 0), offset);
```

### Velocity Control

You can read and set velocities directly:

```cpp
// Linear velocity (movement speed and direction)
body->set_linear_velocity(Vec3(5, 0, 0));
Vec3 vel = body->linear_velocity();

// Velocity at a specific point on the body
Vec3 vel_at_point = body->linear_velocity_at(Vec3(0.5f, 0, 0));

// Angular velocity (rotation speed)
body->set_angular_velocity(Vec3(0, 1, 0));
Vec3 ang_vel = body->angular_velocity();
```

### Damping

Damping simulates resistance like air friction:

```cpp
// Linear damping slows movement over time
body->set_linear_damping(0.1f);

// Angular damping slows rotation
body->set_angular_damping(0.05f);  // Uniform on all axes
body->set_angular_damping(Vec3(0.05f, 0.1f, 0.05f));  // Per-axis
```

### Direction Helpers

Get direction vectors relative to the body's orientation:

```cpp
Vec3 fwd = body->forward();  // Local forward direction in world space
Vec3 rgt = body->right();    // Local right direction in world space
Vec3 up = body->up();        // Local up direction in world space
```

### Locking Rotation

Prevent a body from rotating on specific axes:

```cpp
// Lock rotation on X and Z (useful for 2D-style gameplay in 3D)
body->lock_rotation(true, false, true);
```

### Mass and Center of Mass

```cpp
// Get current mass
float m = body->mass();

// Set mass directly
body->set_mass(5.0f);

// Set custom center of mass (affects how forces cause rotation)
body->set_center_of_mass(Vec3(0, -0.2f, 0));  // Lower CoM = more stable

// Read center of mass in local space
Vec3 com = body->center_of_mass();

// Read center of mass in world space
Vec3 abs_com = body->absolute_center_of_mass();
```

### Sleep State

Bodies that come to rest can "sleep" to save CPU:

```cpp
bool awake = body->is_awake();

// Adjust when a body is allowed to sleep (higher = sleeps sooner)
body->set_angular_sleep_tolerance(0.01f);
```

---

## 3. StaticBody

A `StaticBody` is an immovable collider. It never moves under physics simulation, but dynamic bodies collide with it and respond to it.

### Creating a StaticBody

```cpp
auto ground = create_child<Actor>();
auto mesh = assets->new_mesh_from_procedural_cube();
ground->set_mesh(mesh->id());
ground->scale_by(20, 0.5f, 20);
ground->move_to(0, -1, 0);

auto body = ground->create_child<StaticBody>();

// Use a stone material for heavy, non-bouncy ground
PhysicsMaterial material = PhysicsMaterial::stone();
body->add_box_collider(Vec3(20, 0.5f, 20), material);
```

### Mesh Colliders

Static bodies can use triangle mesh colliders for complex geometry:

```cpp
auto mesh = assets->load_mesh("models/terrain.obj");

auto body = create_child<StaticBody>();
PhysicsMaterial material = PhysicsMaterial::stone();
body->add_mesh_collider(mesh, material);
```

You can also position, orient, and scale the mesh collider independently:

```cpp
body->add_mesh_collider(
    mesh,
    material,
    0,                          // kind (collision layer)
    Vec3(0, 0, 0),             // position offset
    Quaternion(),              // orientation offset
    Vec3(2, 1, 2)              // scale
);
```

### StaticBody Properties

Static bodies support parameters through the stage node system:

```cpp
// When creating via prefab or script, these parameters are available:
// "mesh"        - Source mesh for a mesh collider
// "density"     - Density of the static body material (default: 0.1f)
// "friction"    - Friction of the static body material (default: 0.2f)
// "bounciness"  - Bounciness of the static body material (default: 0.00001f)
```

### When to Use StaticBody

- Ground, floors, and terrain
- Walls and ceilings
- Platforms that do not move
- Trigger zones (using sensor fixtures)
- Architectural geometry

**Important:** Static bodies cannot be moved at runtime. If you need a moving platform or door, use `KinematicBody` instead.

---

## 4. KinematicBody

A `KinematicBody` is controlled entirely by your code. It does not respond to forces or gravity, but it will push dynamic bodies out of its way during collisions.

### Creating a KinematicBody

```cpp
class ElevatorBehaviour : public StageNode {
public:
    FindResult<KinematicBody> body = smlt::FindDescendent<KinematicBody>(this);

    Vec3 bottom_pos = Vec3(0, 0, 0);
    Vec3 top_pos = Vec3(0, 10, 0);
    float t = 0.0f;
    bool going_up = true;

    void on_load() override {
        body->add_box_collider(Vec3(2, 0.2f, 2), PhysicsMaterial::iron());
    }

    void on_update(float dt) override {
        t += going_up ? dt * 0.5f : -dt * 0.5f;
        t = smlt::clamp(t, 0.0f, 1.0f);

        if (t >= 1.0f) going_up = false;
        if (t <= 0.0f) going_up = true;

        Vec3 pos = smlt::lerp(bottom_pos, top_pos, t);
        body->set_position(pos);
    }
};
```

### How Kinematic Bodies Work

Kinematic bodies use a sweep-based approach: when you set their position or velocity, the physics engine calculates how dynamic bodies should respond. This makes them ideal for:

- Moving platforms and elevators
- Conveyor belts
- Doors (sliding or rotating)
- Scripted animations that need physics interaction
- Player characters (when you want direct control over movement)

### Setting Velocity

Instead of setting position directly, you can set velocity for smoother interaction:

```cpp
void on_update(float dt) override {
    // Move upward at 2 units per second
    body->set_linear_velocity(Vec3(0, 2, 0));
}
```

Using velocity instead of position gives the physics engine better information about the body's intended motion, resulting in more stable interactions with dynamic bodies.

---

## 5. Collision Detection

All body types can detect collisions through signals or listeners.

### Using Signals

```cpp
body->signal_collision_enter().connect([this](const Collision& collision) {
    S_INFO("Hit body: {}", collision.other_collider_name);

    for (const auto& cp : collision.contact_points) {
        S_INFO("  Contact at: {} normal: {}", cp.point, cp.normal);
    }
});

body->signal_collision_exit().connect([this](const Collision& collision) {
    S_INFO("Stopped hitting: {}", collision.other_collider_name);
});
```

### Using CollisionListener

For more complex collision handling, subclass `CollisionListener`:

```cpp
class MyCollisionListener : public CollisionListener {
protected:
    void on_collision_enter(const Collision& collision) override {
        S_INFO("Collision with: {}", collision.other_collider_name);
    }

    void on_collision_stay() override {
        // Called every step while in contact
    }

    void on_collision_exit(const Collision& collision) override {
        S_INFO("No longer hitting: {}", collision.other_collider_name);
    }

    // Trigger-specific callbacks (for sensor fixtures)
    void on_trigger_enter() override {}
    void on_trigger_stay() override {}
    void on_trigger_exit() override {}
};

// Register the listener
auto listener = std::make_unique<MyCollisionListener>();
body->register_collision_listener(listener.get());
```

### The Collision Struct

Collision events provide detailed information:

```cpp
struct Collision {
    PhysicsBody* other_body;        // The body we collided with
    std::string other_collider_name; // Name of the other collider
    PhysicsBody* this_body;          // This body
    std::string this_collider_name;  // Our collider name
    std::vector<ContactPoint> contact_points; // Detailed contact info
};

struct ContactPoint {
    Vec3 normal;      // Contact normal (pointing away from other body)
    Vec3 point;       // Contact point in world space
    float separation; // Penetration depth (negative = overlapping)
    PhysicsBody* other_body;
    std::string other_collider;
};
```

### Contact List

Query current contacts on any body:

```cpp
ContactList& contacts = body->contacts();

for (auto it = contacts.begin(); it != contacts.end(); ++it) {
    Contact contact = *it;
    // Inspect current contact...
}

std::size_t count = contacts.count();
smlt::optional<Contact> first = contacts[0];
```

---

## 6. Physics Materials

Materials define the physical properties of a surface. They are passed to collider creation methods:

```cpp
// Built-in materials
PhysicsMaterial wood = PhysicsMaterial::wood();      // density: 0.005, friction: 0.4, bounciness: 0.2
PhysicsMaterial rubber = PhysicsMaterial::rubber();   // density: 0.001, friction: 0.3, bounciness: 0.8
PhysicsMaterial iron = PhysicsMaterial::iron();       // density: 0.1, friction: 0.2, bounciness: ~0
PhysicsMaterial stone = PhysicsMaterial::stone();     // density: 0.1, friction: 0.8, bounciness: ~0

// Custom material
PhysicsMaterial bouncy(0.002f, 0.1f, 0.9f);  // density, friction, bounciness

// With density multiplier
PhysicsMaterial heavy_wood = PhysicsMaterial::wood(2.0f);  // 2x density
```

### Material Properties

| Property | Effect | Typical Range |
|----------|--------|---------------|
| `density` | Mass per unit volume | 0.001 (light foam) to 0.1+ (metal) |
| `friction` | Surface resistance | 0.0 (ice) to 1.0 (rubber) |
| `bounciness` | Coefficient of restitution | 0.0 (dead) to 1.0 (superball) |

---

## 7. Collider Shapes on Bodies

All body types support the same collider shapes:

```cpp
PhysicsMaterial mat = PhysicsMaterial::wood();

// Box collider (size is half-extents on each axis)
body->add_box_collider(Vec3(1, 0.5f, 1), mat);

// Box collider with offset and rotation
body->add_box_collider(
    Vec3(1, 0.5f, 1),
    mat,
    0,                              // kind (collision layer)
    Vec3(0, 0.5f, 0),              // offset from body center
    Quaternion::from_axis_angle(Vec3(1, 0, 0), 45)  // rotation
);

// Sphere collider (diameter, not radius)
body->add_sphere_collider(1.0f, mat);

// Sphere collider with offset
body->add_sphere_collider(1.0f, mat, 0, Vec3(0, 0.5f, 0));

// Capsule collider (defined by two endpoints and diameter)
body->add_capsule_collider(
    Vec3(0, -0.5f, 0),   // Bottom endpoint
    Vec3(0, 0.5f, 0),    // Top endpoint
    0.3f,                // Diameter
    mat
);

// Triangle collider (single triangle)
body->add_triangle_collider(
    Vec3(-1, 0, -1),
    Vec3(1, 0, -1),
    Vec3(0, 0, 1),
    mat
);
```

### Multiple Fixtures

A single body can have multiple fixtures:

```cpp
auto body = actor->create_child<DynamicBody>();

// Main body collider
body->add_box_collider(Vec3(0.5f, 1, 0.5f), PhysicsMaterial::wood());

// Feet sensor (for ground detection)
PhysicsMaterial sensor_mat;
body->add_box_collider(Vec3(0.4f, 0.1f, 0.4f), sensor_mat, 0, Vec3(0, -1.05f, 0));
```

The `kind` parameter on collider methods is a `uint16_t` that can be used with a custom `ContactFilter` to control which fixtures collide with each other. See the [Colliders](colliders.md) guide for details.

---

## 8. Transform Synchronization

Physics bodies automatically synchronize their visual transforms with the physics simulation. The `PhysicsBody` base class handles this in `on_update()`:

```cpp
void PhysicsBody::on_update(float dt) {
    if (transform->smoothing_mode() == TRANSFORM_SMOOTHING_EXTRAPOLATE) {
        // Interpolate between previous and current physics states
        // for smooth rendering at variable frame rates
        auto prev_state = last_state_;
        auto next_state = std::make_pair(position(), orientation());

        float r = get_app()->time_keeper->fixed_step_remainder();
        float t = (dt == 0.0f) ? 0.0f : smlt::fast_divide(r, dt);

        auto new_pos = prev_state.first.lerp(next_state.first, t);
        auto new_rot = prev_state.second.slerp(next_state.second, t);

        transform->set_position(new_pos);
        transform->set_orientation(new_rot);
    } else {
        // Direct copy of physics state
        transform->set_position(position());
        transform->set_orientation(orientation());
    }
}
```

### Extrapolation Smoothing

For the smoothest visual result, enable extrapolation smoothing on the visual node that owns the physics body:

```cpp
actor->transform->set_smoothing_mode(TRANSFORM_SMOOTHING_EXTRAPOLATE);
```

This interpolates between the previous and current physics states based on the frame remainder, eliminating the "stutter" that can occur when physics runs at a fixed timestep but rendering runs at a variable rate.

---

## 9. Body Lifecycle

### Creation Order

Always create the visual node first, then attach the physics body as a child:

```cpp
auto actor = create_child<Actor>();
actor->set_mesh(mesh->id());

auto body = actor->create_child<DynamicBody>();
body->add_box_collider(Vec3(1, 1, 1), PhysicsMaterial::wood());
body->set_mass(1.0f);
```

This ensures the physics body inherits the visual node's transform.

### Destruction

When a node is destroyed, its physics body is automatically unregistered from the simulation:

```cpp
actor->destroy();  // Body is automatically removed
```

You do not need to manually remove bodies from the simulation. However, if you destroy a body that has joints, the joints are also cleaned up:

```cpp
// ReactiveBody::on_destroy() automatically:
// 1. Removes all sphere joints from connected bodies
// 2. Destroys all sphere joints
// 3. Unregisters the body from the simulation
```

---

## 10. Choosing the Right Body Type

| Scenario | Body Type |
|----------|-----------|
| Falling crate | `DynamicBody` |
| Bullet (physics-reactive) | `DynamicBody` |
| Ragdoll | `DynamicBody` |
| Vehicle chassis | `DynamicBody` |
| Ground plane | `StaticBody` |
| Building walls | `StaticBody` |
| Terrain mesh | `StaticBody` |
| Moving platform | `KinematicBody` |
| Elevator | `KinematicBody` |
| Sliding door | `KinematicBody` |
| Player character (direct control) | `KinematicBody` |
| Conveyor belt | `KinematicBody` |

---

## See Also

- [Physics Overview](overview.md) -- General physics introduction
- [Colliders](colliders.md) -- Fixture shapes, materials, and collision filtering
- [Joints](joints.md) -- Connecting bodies together
- [Raycasting](raycasting.md) -- Querying the physics world
- [Best Practices](best-practices.md) -- Optimization and patterns
