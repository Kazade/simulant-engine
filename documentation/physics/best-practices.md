# Physics Best Practices

This guide covers optimization techniques, common patterns, and pitfalls when working with Simulant's physics system. Following these practices will help you maintain good performance and avoid common bugs.

**Related documentation:** [Physics Overview](overview.md), [Rigid Bodies](rigid-bodies.md), [Colliders](colliders.md), [Joints](joints.md), [Raycasting](raycasting.md).

---

## 1. Performance Optimization

### 1.1 Collider Complexity

The biggest factor in physics performance is collider complexity. Shapes are ordered from cheapest to most expensive:

```
Sphere < Capsule < Box < Triangle < Mesh
```

**Do:**

```cpp
// Use the simplest shape that works
body->add_sphere_collider(radius, mat);       // Fastest
body->add_box_collider(size, mat);            // Fast
body->add_capsule_collider(v0, v1, d, mat);   // Good for characters
```

**Avoid:**

```cpp
// Mesh colliders are very expensive
body->add_mesh_collider(complex_mesh, mat);   // Only on StaticBody!
```

### 1.2 Guidelines for Collider Selection

| Object Type | Recommended Collider |
|-------------|---------------------|
| Ball, orb, coin | Sphere |
| Crate, box, wall | Box |
| Character, NPC | Capsule |
| Projectile (small) | Sphere |
| Vehicle | Multiple boxes or capsules |
| Terrain, building | Mesh (StaticBody only) |
| Trigger zone | Sphere or Box |

### 1.3 Reduce Collider Count

Each body should have the minimum number of fixtures needed:

```cpp
// Good: Single box for a crate
body->add_box_collider(Vec3(1, 1, 1), mat);

// Bad: Excessive fixtures for a simple box
body->add_box_collider(Vec3(1, 1, 1), mat);
body->add_box_collider(Vec3(1, 1, 1), mat, 0, Vec3(0.01f, 0, 0));  // Redundant
body->add_box_collider(Vec3(1, 1, 1), mat, 0, Vec3(-0.01f, 0, 0)); // Redundant
```

### 1.4 Mass Ratios

Keep mass ratios between connected bodies reasonable:

```cpp
// Good: 5:1 ratio
body_a->set_mass(10.0f);
body_b->set_mass(2.0f);

// Bad: 1000:1 ratio causes instability
body_a->set_mass(1000.0f);
body_b->set_mass(1.0f);
```

As a rule of thumb, keep mass ratios under **10:1** for stable simulation.

### 1.5 Sleeping

Bodies that come to rest automatically "sleep" to save CPU. You can influence this:

```cpp
// Allow bodies to sleep sooner
body->set_angular_sleep_tolerance(0.05f);  // Higher = sleeps easier

// Check if a body is sleeping
if (!body->is_awake()) {
    // Body is at rest, skip expensive logic
}

// Wake a body manually if needed
body->add_impulse(Vec3(0, 1, 0));  // Any force wakes a body
```

---

## 2. Scene Setup

### 2.1 Always Use a PhysicsService

The physics system requires a `PhysicsService` in your scene. Set it up early:

```cpp
class GameScene : public Scene<GameScene> {
public:
    void on_init() override {
        // Create physics service before any bodies
        auto physics = create_service<PhysicsService>();
        physics->set_gravity(Vec3(0, -9.81f, 0));

        // Now it's safe to create physics bodies
        create_world();
    }
};
```

### 2.2 Gravity

Set gravity to match your game's needs:

```cpp
// Earth gravity
physics->set_gravity(Vec3(0, -9.81f, 0));

// Low gravity (moon-like)
physics->set_gravity(Vec3(0, -1.62f, 0));

// No gravity (top-down game)
physics->set_gravity(Vec3(0, 0, 0));

// Custom direction
physics->set_gravity(Vec3(0, 0, -9.81f));  // "Down" is -Z
```

---

## 3. Common Patterns

### 3.1 Character Controller

```cpp
class CharacterController : public StageNode {
public:
    FindResult<KinematicBody> body = smlt::FindDescendent<KinematicBody>(this);
    PhysicsService* physics_ = nullptr;

    float move_speed = 5.0f;
    float jump_force = 8.0f;
    bool is_jumping_ = false;

    void on_load() override {
        physics_ = scene->find_service<PhysicsService>();

        // Capsule collider
        body->add_capsule_collider(
            Vec3(0, -0.4f, 0),
            Vec3(0, 0.4f, 0),
            0.3f,
            PhysicsMaterial::wood()
        );

        // Ground sensor
        constexpr uint16_t SENSOR = 1 << 15;
        body->add_sphere_collider(0.1f, PhysicsMaterial(), SENSOR, Vec3(0, -0.5f, 0));
    }

    void on_fixed_update(float step) override {
        Vec3 input = get_movement_input();  // Your input handling

        if (input.length_squared() > 0) {
            input = input.normalized() * move_speed;
            body->set_linear_velocity(Vec3(input.x, body->linear_velocity().y, input.z));
        }

        // Jump
        if (want_jump() && is_grounded()) {
            body->set_linear_velocity(
                Vec3(body->linear_velocity().x, jump_force, body->linear_velocity().z)
            );
            is_jumping_ = true;
        }
    }

    bool is_grounded() {
        if (!physics_) return false;

        Vec3 origin = body->absolute_center_of_mass();
        auto result = physics_->ray_cast(origin, Vec3(0, -1, 0), 0.6f);
        return result.has_value();
    }
};
```

### 3.2 Moving Platform

```cpp
class MovingPlatform : public StageNode {
public:
    FindResult<KinematicBody> body = smlt::FindDescendent<KinematicBody>(this);

    Vec3 point_a = Vec3(-5, 0, 0);
    Vec3 point_b = Vec3(5, 0, 0);
    float speed = 2.0f;
    float t_ = 0.0f;
    bool forward_ = true;

    void on_load() override {
        body->add_box_collider(Vec3(2, 0.2f, 2), PhysicsMaterial::iron());
        body->set_position(point_a);
    }

    void on_fixed_update(float step) override {
        t_ += forward_ ? step * speed : -step * speed;
        t_ = smlt::clamp(t_, 0.0f, 1.0f);

        if (t_ >= 1.0f) forward_ = false;
        if (t_ <= 0.0f) forward_ = true;

        Vec3 pos = smlt::lerp(point_a, point_b, t_);
        body->set_position(pos);
    }
};
```

**Key insight:** Use `set_position()` for kinematic bodies rather than `set_linear_velocity()` when you need precise positioning. However, `set_linear_velocity()` gives the physics engine better information for resolving collisions with dynamic bodies.

### 3.3 Destructible Objects

```cpp
class DestructibleCrate : public StageNode {
public:
    FindResult<DynamicBody> body = smlt::FindDescendent<DynamicBody>(this);
    float health_ = 100.0f;

    void on_load() override {
        body->signal_collision_enter().connect([this](const Collision& collision) {
            // Calculate impact force from velocity change
            Vec3 impact_velocity = body->linear_velocity();
            float force = impact_velocity.length() * body->mass();

            take_damage(force * 10.0f);
        });
    }

    void take_damage(float amount) {
        health_ -= amount;
        if (health_ <= 0) {
            destroy();
            spawn_debris();
        }
    }

    void spawn_debris() {
        // Spawn smaller pieces with their own physics
        for (int i = 0; i < 4; ++i) {
            auto piece = create_child<Actor>();
            auto mesh = assets->new_mesh_from_procedural_cube();
            piece->set_mesh(mesh->id());
            piece->scale_by(0.3f, 0.3f, 0.3f);
            piece->move_to(
                transform->position().x + random_float(-0.5f, 0.5f),
                transform->position().y + random_float(-0.5f, 0.5f),
                transform->position().z + random_float(-0.5f, 0.5f)
            );

            auto piece_body = piece->create_child<DynamicBody>();
            piece_body->add_box_collider(Vec3(0.15f, 0.15f, 0.15f), PhysicsMaterial::wood());
            piece_body->set_mass(0.5f);
            piece_body->add_impulse(Vec3(
                random_float(-5, 5),
                random_float(5, 15),
                random_float(-5, 5)
            ));
        }
    }
};
```

### 3.4 Explosion Force

```cpp
void apply_explosion_force(
    Vec3 center, float radius, float force,
    const std::vector<DynamicBody*>& bodies
) {
    for (auto body : bodies) {
        if (!body->is_awake()) continue;  // Skip sleeping bodies

        Vec3 body_pos = body->absolute_center_of_mass();
        Vec3 direction = body_pos - center;
        float distance = direction.length();

        if (distance < radius && distance > 0.001f) {
            direction = direction.normalized();

            // Force decreases with distance
            float falloff = 1.0f - (distance / radius);
            float applied_force = force * falloff;

            body->add_impulse_at_position(
                direction * applied_force,
                body_pos
            );
        }
    }
}
```

### 3.5 Conveyor Belt

```cpp
class ConveyorBelt : public StageNode {
public:
    FindResult<KinematicBody> belt = smlt::FindDescendent<KinematicBody>(this);
    Vec3 belt_velocity = Vec3(2, 0, 0);  // Moving along X

    void on_load() override {
        belt->add_box_collider(Vec3(3, 0.1f, 1), PhysicsMaterial::rubber());
    }

    void on_fixed_update(float step) override {
        // Set velocity to push objects that land on it
        belt->set_linear_velocity(belt_velocity);
    }
};
```

---

## 4. Collision Handling

### 4.1 Using Signals

Signals are the simplest way to react to collisions:

```cpp
body->signal_collision_enter().connect([this](const Collision& collision) {
    S_INFO("Hit: {}", collision.other_collider_name);
});

body->signal_collision_exit().connect([this](const Collision& collision) {
    S_INFO("Stopped hitting: {}", collision.other_collider_name);
});
```

### 4.2 Using CollisionListener

For complex logic, subclass `CollisionListener`:

```cpp
class EnemyCollisionListener : public CollisionListener {
    EnemyBehaviour* enemy_;

public:
    EnemyCollisionListener(EnemyBehaviour* enemy) : enemy_(enemy) {}

protected:
    void on_collision_enter(const Collision& collision) override {
        // Check if hit by a bullet
        if (collision.other_collider_name == "Bullet") {
            enemy_->take_damage(25.0f);
        }
    }

    void on_trigger_enter() override {
        // Player entered trigger zone
        enemy_->start_chasing();
    }
};

// Register it
auto listener = std::make_unique<EnemyCollisionListener>(this);
body->register_collision_listener(listener.get());
```

### 4.3 Inspecting Collision Details

```cpp
void on_collision_enter(const Collision& collision) override {
    // Which body did we hit?
    PhysicsBody* other = collision.other_body;

    // What collider on that body?
    std::string collider_name = collision.other_collider_name;

    // Contact points (for detailed collision info)
    for (const auto& contact : collision.contact_points) {
        Vec3 normal = contact.normal;       // Surface normal
        Vec3 point = contact.point;         // Hit position
        float separation = contact.separation; // Penetration depth
    }
}
```

---

## 5. Debugging Physics

### 5.1 Debug Visualization

Enable physics debug rendering to see colliders, bodies, and joints:

```cpp
auto debug = create_child<Debug>();
physics_service->set_debug(debug);
```

### 5.2 Logging Collisions

Add logging to track unexpected collisions:

```cpp
body->signal_collision_enter().connect([](const Collision& collision) {
    S_DEBUG("Collision: {} <-> {}",
        collision.this_collider_name,
        collision.other_collider_name
    );
});
```

### 5.3 Checking Body State

```cpp
// Is the body active?
if (body->is_awake()) {
    Vec3 vel = body->linear_velocity();
    Vec3 ang_vel = body->angular_velocity();
    Vec3 pos = body->position();
    Quaternion rot = body->orientation();

    S_DEBUG("Body: pos={} vel={}", pos, vel);
}
```

### 5.4 Direction Vectors

```cpp
// Get body-relative directions
Vec3 forward = body->forward();  // Local forward in world space
Vec3 right = body->right();      // Local right in world space
Vec3 up = body->up();            // Local up in world space

// Useful for forces in local space
body->add_relative_force(Vec3(0, 0, 10));  // "Forward" thrust
```

---

## 6. Common Pitfalls

### 6.1 Bodies Falling Through Ground

**Cause:** Ground has no collider, or collider is too thin.

**Fix:**

```cpp
// Ensure ground has a StaticBody with a proper collider
auto ground = create_child<StaticBody>();
ground->add_box_collider(Vec3(50, 1, 50), PhysicsMaterial::stone());
```

### 6.2 Bodies Not Colliding

**Cause:** Contact filter is preventing the collision.

**Fix:** Check your `ContactFilter::should_collide()` logic. Ensure kind values are set correctly.

```cpp
// Verify kind values
body->add_box_collider(size, mat, PLAYER);  // Not 0 if you filter on 0
```

### 6.3 Jittery Physics

**Cause:** Mass ratios too extreme, or too many colliders.

**Fix:** Reduce mass ratios, simplify colliders.

```cpp
// Bad: 1000:1 ratio
anchor->set_mass(1000.0f);
pendulum->set_mass(1.0f);

// Better: 100:1 ratio
anchor->set_mass(100.0f);
pendulum->set_mass(1.0f);
```

### 6.4 Transform Mismatch

**Cause:** Visual and physics transforms are out of sync.

**Fix:** The physics body should be a child of the visual node:

```cpp
auto actor = create_child<Actor>();    // Visual
actor->set_mesh(mesh->id());

auto body = actor->create_child<DynamicBody>();  // Physics as child
body->add_box_collider(size, mat);
// Body automatically follows actor's transform
```

### 6.5 Ray Origin Inside Collider

**Cause:** Ray starts inside a body, causing incorrect results.

**Fix:** Offset the ray origin:

```cpp
Vec3 origin = muzzle->transform->world_position();
Vec3 direction = muzzle->transform->world_forward().normalized();

// Offset origin slightly
auto result = physics_service_->ray_cast(
    origin + direction * 0.1f,  // Offset
    direction,
    100.0f
);
```

### 6.6 Forgetting PhysicsService

**Cause:** Creating bodies before the `PhysicsService` exists.

**Fix:** Always create the service first:

```cpp
void on_init() override {
    create_service<PhysicsService>();  // First!
    physics_service_->set_gravity(Vec3(0, -9.81f, 0));

    create_bodies();  // Then create bodies
}
```

---

## 7. Organization Tips

### 7.1 Collision Layers

Use kind values as bit flags for clean collision layers:

```cpp
enum CollisionLayer : uint16_t {
    LAYER_TERRAIN   = 1 << 0,
    LAYER_PLAYER    = 1 << 1,
    LAYER_ENEMY     = 1 << 2,
    LAYER_BULLET    = 1 << 3,
    LAYER_PICKUP    = 1 << 4,
    LAYER_TRIGGER   = 1 << 15,
};
```

### 7.2 Collision Matrix

Document which layers collide:

```
            | Terrain | Player | Enemy | Bullet | Pickup | Trigger |
------------|---------|--------|-------|--------|--------|---------|
Terrain     |    X    |   X    |   X   |   X    |   X    |    X    |
Player      |    X    |        |   X   |   X    |   X    |    X    |
Enemy       |    X    |   X    |       |   X    |        |    X    |
Bullet      |    X    |   X    |   X   |        |        |    X    |
Pickup      |    X    |   X    |       |        |        |    X    |
Trigger     |    X    |   X    |   X   |   X    |   X    |         |
```

### 7.3 Prefabs for Common Setups

Create reusable prefab patterns:

```cpp
DynamicBody* create_physics_crate(Vec3 position, float size = 1.0f) {
    auto actor = create_child<Actor>();
    auto mesh = assets->new_mesh_from_procedural_cube();
    actor->set_mesh(mesh->id());
    actor->scale_by(size, size, size);
    actor->move_to(position);

    auto body = actor->create_child<DynamicBody>();
    body->add_box_collider(Vec3(size/2, size/2, size/2), PhysicsMaterial::wood());
    body->set_mass(size * size * size);  // Mass proportional to volume

    return body;
}

StaticBody* create_physics_wall(Vec3 position, Vec3 size) {
    auto actor = create_child<Actor>();
    auto mesh = assets->new_mesh_from_procedural_cube();
    actor->set_mesh(mesh->id());
    actor->scale_by(size.x, size.y, size.z);
    actor->move_to(position);

    auto body = actor->create_child<StaticBody>();
    body->add_box_collider(size / 2, PhysicsMaterial::stone());

    return body;
}
```

---

## 8. Checklist

Before shipping a physics-heavy scene:

- [ ] All static geometry has `StaticBody` colliders
- [ ] Dynamic objects have appropriate mass (no extreme ratios)
- [ ] Collider shapes are as simple as possible
- [ ] No mesh colliders on dynamic bodies
- [ ] Contact filter correctly configured
- [ ] Gravity set appropriately
- [ ] Joints clean up when bodies are destroyed
- [ ] Raycasts have reasonable maximum distances
- [ ] Physics debug visualization tested
- [ ] Performance tested with maximum expected object count

---

## See Also

- [Physics Overview](overview.md) -- General physics introduction
- [Rigid Bodies](rigid-bodies.md) -- Body types and properties
- [Colliders](colliders.md) -- Collider shapes and materials
- [Joints](joints.md) -- Connecting bodies together
- [Raycasting](raycasting.md) -- Querying the physics world
