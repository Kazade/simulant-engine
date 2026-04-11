# Physics Overview

Simulant includes a rigid body physics system built on the **Bounce** physics library. This allows you to simulate realistic physics in your games including gravity, collisions, joints, and forces.

## What is Physics Simulation?

Physics simulation adds realistic physical behavior to objects in your game. Instead of manually moving objects, you:

1. Create physics bodies with properties (mass, shape, etc.)
2. Apply forces (gravity, impulses, etc.)
3. Let the physics engine calculate movement and collisions
4. Read the results to update visual positions

## Physics Architecture

In Simulant, physics is implemented using **Behaviours** - reusable components that attach to StageNodes:

```
StageNode (visual)
└── Physics Behaviour (physical properties)
    ├── DynamicBody - Full simulation
    ├── StaticBody - Immovable obstacle
    ├── KinematicBody - Script-controlled
    └── ReactiveBody - Responds to forces
```

### Key Classes

| Class | Purpose |
|-------|---------|
| `RigidBodySimulation` | The physics world/manager |
| `DynamicBody` | Physics object affected by forces |
| `StaticBody` | Immovable collider (walls, floors) |
| `KinematicBody` | Script-controlled collider (moving platforms) |
| `Fixture` | Collider shape (box, sphere, capsule, triangle) |
| `SphereJoint` | Connect two bodies with constraints |
| `PhysicsScene` | Helper class that sets up physics for a scene |

## Quick Start

### 1. Create a Physics Scene

The easiest way to get started is using `PhysicsScene`:

```cpp
class GameScene : public Scene<GameScene> {
private:
    PhysicsScene* physics_scene_;
    
    void on_load() override {
        // Create physics simulation
        physics_scene_ = create_child<PhysicsScene>();
        physics_scene_->set_gravity(Vec3(0, -9.81f, 0));  // Earth gravity
        
        create_ground();
        create_falling_boxes();
    }
};
```

### 2. Create Ground

Static bodies don't move - perfect for ground and walls:

```cpp
void create_ground() {
    auto ground = create_child<Actor>();
    auto ground_mesh = assets->new_mesh_from_procedural_cube();
    ground->set_mesh(ground_mesh->id());
    ground->scale_by(10, 0.1f, 10);
    ground->move_to(0, -1, 0);
    
    // Add static physics body
    auto static_body = ground->create_child<StaticBody>();
    static_body->create_box_fixture(ground_mesh->bounding_box());
}
```

### 3. Create Dynamic Objects

Dynamic bodies fall, bounce, and collide:

```cpp
void create_falling_boxes() {
    auto sim = physics_scene_->simulation();
    
    for (int i = 0; i < 5; ++i) {
        auto box = create_child<Actor>();
        auto mesh = assets->new_mesh_from_procedural_cube();
        box->set_mesh(mesh->id());
        box->move_to(i * 2 - 4, 10, 0);
        
        // Add dynamic physics body
        auto body = box->create_child<DynamicBody>();
        body->create_box_fixture(mesh->bounding_box());
        body->set_mass(1.0f);
        body->set_restitution(0.5f);  // Bounciness
    }
}
```

## Physics Bodies Explained

### DynamicBody

A fully simulated physics object:

```cpp
auto body = node->create_child<DynamicBody>();
body->create_box_fixture(size);
body->set_mass(1.0f);

// Properties
body->set_restitution(0.8f);      // Bounciness (0-1)
body->set_friction(0.5f);         // Surface friction
body->set_linear_damping(0.1f);   // Air resistance
body->set_angular_damping(0.1f);  // Rotational damping
```

**Use for**: Balls, crates, characters, debris, anything that should react to physics.

### StaticBody

An immovable collider:

```cpp
auto body = node->create_child<StaticBody>();
body->create_box_fixture(size);

// Properties
body->set_restitution(0.3f);
body->set_friction(0.8f);
```

**Use for**: Ground, walls, platforms, triggers (with sensors).

### KinematicBody

A body controlled by script, not physics:

```cpp
auto body = node->create_child<KinematicBody>();
body->create_box_fixture(size);

// Move it manually - physics will respond
void on_update(float dt) override {
    body->set_linear_velocity(Vec3(1, 0, 0));
}
```

**Use for**: Moving platforms, elevators, doors, animated objects that should push other bodies.

## Collider Shapes (Fixtures)

### Box Collider

```cpp
body->create_box_fixture(Vec3(width, height, depth));
body->create_box_fixture(aabb);  // From mesh bounding box
```

### Sphere Collider

```cpp
body->create_sphere_fixture(radius);
```

### Capsule Collider

```cpp
body->create_capsule_fixture(radius, height);
```

### Triangle Mesh Collider

```cpp
// For complex static geometry
body->create_triangle_mesh_fixture(mesh);
```

## Physics Properties

### Mass

```cpp
body->set_mass(2.0f);           // Set mass
float mass = body->mass();      // Get mass
body->set_mass_from_density();  // Auto-calculate from volume
```

### Velocity

```cpp
// Linear velocity (movement)
body->set_linear_velocity(Vec3(1, 0, 0));
Vec3 vel = body->linear_velocity();

// Angular velocity (rotation)
body->set_angular_velocity(Vec3(0, 1, 0));
Vec3 ang_vel = body->angular_velocity();
```

### Forces

```cpp
// Apply force at center of mass
body->apply_force(Vec3(0, 100, 0));

// Apply force at specific point
body->apply_force(Vec3(0, 100, 0), application_point);

// Apply impulse (instant velocity change)
body->apply_impulse(Vec3(0, 50, 0));

// Apply torque (rotational force)
body->apply_torque(Vec3(0, 10, 0));
```

## Collision Detection

### Collision Listeners

React to collisions:

```cpp
class BounceBehaviour : public StageNode {
public:
    FindResult<DynamicBody> body = FindDescendent("Body", this);
    
    void on_load() override {
        body->signal_contact_begin().connect([this](FixturePtr self, FixturePtr other) {
            S_INFO("Collision started!");
            on_hit();
        });
        
        body->signal_contact_end().connect([this](FixturePtr self, FixturePtr other) {
            S_INFO("Collision ended!");
        });
    }
};
```

### Sensors

Fixtures can be sensors - they detect overlaps without colliding:

```cpp
auto fixture = body->create_box_fixture(size);
fixture->set_is_sensor(true);

// Detect when objects enter
body->signal_contact_begin().connect([](FixturePtr self, FixturePtr other) {
    S_INFO("Object entered trigger zone");
});
```

**Use for**: Trigger zones, pickup items, checkpoints.

## Raycasting

Cast rays to query the physics world:

```cpp
auto sim = physics_scene_->simulation();

// Cast ray
Ray ray(origin, direction);
PhysicsRaycastHit hit;

if (sim->raycast(ray, 100.0f, hit)) {
    S_INFO("Hit something at distance: {}", hit.distance);
    S_INFO("Hit point: {}", hit.point);
    S_INFO("Hit normal: {}", hit.normal);
}
```

### Raycast Options

Filter what the ray hits:

```cpp
PhysicsRaycastOptions opts;
opts.hit_multiple = true;          // Get all hits, not just first
opts.report_initial_position = true;
opts.filter = [](FixturePtr fixture) {
    // Only hit dynamic bodies
    return fixture->body()->is_dynamic();
};

sim->raycast(ray, 100.0f, hit, opts);
```

## Joints

Connect bodies together with constraints:

### Sphere Joint

```cpp
auto joint = sim->create_joint<SphereJoint>();
joint->connect(body_a, body_b);
joint->set_anchor(anchor_point);
joint->set_radius(2.0f);  // How far body_b can move from anchor
```

## Best Practices

### 1. Use PhysicsScene Helper

Always use `PhysicsScene` for easy setup:

```cpp
physics_scene_ = create_child<PhysicsScene>();
physics_scene_->set_gravity(Vec3(0, -9.81f, 0));
```

### 2. Match Visual and Physics

Keep physics bodies aligned with visuals:

```cpp
// Physics body as child of visual
auto actor = create_child<Actor>();
actor->set_mesh(mesh->id());

auto body = actor->create_child<DynamicBody>();
body->create_box_fixture(mesh->bounding_box());
// Body automatically follows visual's transform
```

### 3. Use Appropriate Collider

Simple is better:

```cpp
// Good: Sphere for balls
body->create_sphere_fixture(radius);

// Good: Box for crates
body->create_box_fixture(size);

// Avoid: Triangle mesh for everything
// body->create_triangle_mesh_fixture(mesh);  // Expensive!
```

### 4. Disable When Not Needed

Pause physics for performance:

```cpp
physics_scene_->set_enabled(false);  // Pause
physics_scene_->set_enabled(true);   // Resume
```

### 5. Clean Up

Physics bodies are destroyed with their nodes:

```cpp
actor->destroy();  // Body is automatically removed from simulation
```

## Common Patterns

### Jumping Character

```cpp
void jump(DynamicBody* body) {
    if (body->is_grounded()) {
        body->apply_impulse(Vec3(0, 100, 0));
    }
}
```

### Explosion Force

```cpp
void explosion(Vec3 center, float radius, float force) {
    for (auto body : all_bodies()) {
        float dist = body->distance_to(center);
        if (dist < radius) {
            Vec3 dir = (body->position() - center).normalized();
            float strength = force * (1.0f - dist / radius);
            body->apply_impulse(dir * strength);
        }
    }
}
```

### Moving Platform

```cpp
class MovingPlatform : public StageNode {
public:
    FindResult<KinematicBody> body = FindDescendent("Body", this);
    Vec3 start_pos, end_pos;
    float t = 0;
    bool forward = true;
    
    void on_update(float dt) override {
        t += forward ? dt : -dt;
        if (t >= 1.0f) { t = 1.0f; forward = false; }
        if (t <= 0.0f) { t = 0.0f; forward = true; }
        
        Vec3 pos = lerp(start_pos, end_pos, t);
        body->move_to(pos);
    }
};
```

## Troubleshooting

### Objects Fall Through Ground

- Ensure ground has a StaticBody
- Check fixture sizes match
- Verify collision layers aren't filtering them out

### Performance Issues

- Reduce triangle mesh colliders
- Use simpler collider shapes
- Disable bodies that are sleeping
- Reduce simulation frequency

### Bodies Not Colliding

- Check both bodies have fixtures
- Verify simulation is enabled
- Ensure bodies aren't in same collision group (if using groups)

## Next Steps

- **[Rigid Bodies](rigid-bodies.md)** - Detailed body types
- **[Colliders](colliders.md)** - Fixture shapes and properties
- **[Joints](joints.md)** - Connecting bodies
- **[Raycasting](raycasting.md)** - Physics queries
- **[Best Practices](best-practices.md)** - Optimization tips

## See Also

- **[Stage Nodes](stage-nodes.md)** - Node hierarchy
- **[Behaviours](../scripting/behaviours.md)** - Physics as behaviours
- **[Actors](actors.md)** - Visual representation
