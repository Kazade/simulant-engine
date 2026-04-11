# Raycasting

Raycasting allows you to query the physics world by casting an invisible ray and detecting what it hits. This is essential for shooting, line-of-sight checks, mouse picking, and many other gameplay systems.

**Related documentation:** [Physics Overview](overview.md), [Rigid Bodies](rigid-bodies.md), [Colliders](colliders.md).

---

## 1. What is Raycasting?

Raycasting sends a ray (an origin point and direction) through the physics world and returns information about the first object it hits. Unlike collision events, which are passive, raycasting is an active query you perform on demand.

Common uses include:

- **Shooting** -- detecting what a bullet would hit
- **Line of sight** -- checking if an NPC can see the player
- **Mouse picking** -- determining which 3D object the cursor is over
- **Ground detection** -- casting downward to check if a character is grounded
- **Range checking** -- measuring distance to the nearest obstacle

---

## 2. Basic Raycasting

### 2.1 Accessing the PhysicsService

Raycasting is performed through the `PhysicsService`, which you access from your scene:

```cpp
class ShootingBehaviour : public StageNode {
public:
    void on_load() override {
        physics_service_ = scene->find_service<PhysicsService>();
    }

    void try_shoot(Vec3 origin, Vec3 direction) {
        if (!physics_service_) return;

        auto result = physics_service_->ray_cast(origin, direction);

        if (result) {
            S_INFO("Hit something at distance: {}", result->distance);
            S_INFO("Hit normal: {}", result->normal);
            S_INFO("Impact point: {}", result->impact_point);

            // result->other_body gives you the PhysicsBody that was hit
        } else {
            S_INFO("Ray didn't hit anything");
        }
    }

private:
    PhysicsService* physics_service_ = nullptr;
};
```

### 2.2 RayCastResult

The `ray_cast` method returns `smlt::optional<RayCastResult>`. If the ray hits something, the result contains:

```cpp
struct RayCastResult {
    PhysicsBody* other_body;   // The body that was hit
    float distance;             // Distance from origin to impact
    Vec3 normal;               // Surface normal at impact point
    Vec3 impact_point;         // World-space hit position
};
```

### 2.3 Maximum Distance

You can limit how far the ray travels:

```cpp
// Ray cast up to 50 units
auto result = physics_service_->ray_cast(origin, direction, 50.0f);

// Ray cast indefinitely (default)
auto result = physics_service_->ray_cast(origin, direction);
// Equivalent to:
auto result = physics_service_->ray_cast(
    origin, direction,
    std::numeric_limits<float>::max()
);
```

---

## 3. Raycasting Patterns

### 3.1 Shooting / Hitscan Weapons

```cpp
class WeaponBehaviour : public StageNode {
public:
    FindResult<Actor> muzzle = smlt::FindChild<Actor>("Muzzle", this);
    PhysicsService* physics_service_ = nullptr;

    void on_load() override {
        physics_service_ = scene->find_service<PhysicsService>();
    }

    void fire() {
        if (!physics_service_) return;

        Vec3 origin = muzzle->transform->world_position();
        Vec3 direction = muzzle->transform->world_forward().normalized();

        auto result = physics_service_->ray_cast(origin, direction, 100.0f);

        if (result) {
            apply_damage(result->other_body, result->impact_point);
            spawn_impact_effect(result->impact_point, result->normal);
        } else {
            // Hit nothing -- maybe play a "miss" effect
        }
    }

    void apply_damage(PhysicsBody* body, Vec3 hit_point) {
        // Identify what was hit and apply damage
        S_INFO("Hit body at {}", hit_point);
    }

    void spawn_impact_effect(Vec3 point, Vec3 normal) {
        // Spawn particle effect or decal at hit point
        // Align effect to surface normal
    }
};
```

### 3.2 Line of Sight

```cpp
class NPCVisionBehaviour : public StageNode {
public:
    FindResult<DynamicBody> player_body = smlt::FindDescendent<DynamicBody>("Player", this);
    PhysicsService* physics_service_ = nullptr;

    void on_load() override {
        physics_service_ = scene->find_service<PhysicsService>();
    }

    bool can_see_player() {
        if (!physics_service_ || !player_body) return false;

        Vec3 npc_pos = transform->world_position();
        Vec3 player_pos = player_body->absolute_center_of_mass();

        Vec3 direction = (player_pos - npc_pos).normalized();
        float distance = (player_pos - npc_pos).length();

        auto result = physics_service_->ray_cast(npc_pos, direction, distance);

        // If we hit something, check if it's the player
        if (result) {
            return result->other_body == player_body;
        }

        return false;
    }

    void on_update(float dt) override {
        if (can_see_player()) {
            S_INFO("NPC can see the player!");
        }
    }
};
```

### 3.3 Ground Detection

```cpp
class GroundDetectorBehaviour : public StageNode {
public:
    FindResult<DynamicBody> body = smlt::FindDescendent<DynamicBody>(this);
    PhysicsService* physics_service_ = nullptr;
    float ground_check_distance = 1.1f;

    void on_load() override {
        physics_service_ = scene->find_service<PhysicsService>();
    }

    bool is_grounded() {
        if (!physics_service_ || !body) return false;

        Vec3 origin = body->absolute_center_of_mass();
        Vec3 direction = Vec3(0, -1, 0);  // Straight down

        auto result = physics_service_->ray_cast(
            origin, direction, ground_check_distance
        );

        return result.has_value();
    }

    void on_update(float dt) override {
        if (is_grounded()) {
            S_INFO("Character is on the ground");
        }
    }
};
```

### 3.4 Distance to Nearest Obstacle

```cpp
float distance_to_nearest_obstacle(Vec3 origin, Vec3 direction) {
    auto physics_service = scene->find_service<PhysicsService>();
    if (!physics_service) return std::numeric_limits<float>::max();

    auto result = physics_service->ray_cast(origin, direction);

    if (result) {
        return result->distance;
    }

    return std::numeric_limits<float>::max();
}
```

### 3.5 Mouse Picking (3D Selection)

```cpp
class PickingBehaviour : public StageNode {
public:
    FindResult<Camera> camera = smlt::FindChild<Camera>("MainCamera", this);
    PhysicsService* physics_service_ = nullptr;

    void on_load() override {
        physics_service_ = scene->find_service<PhysicsService>();
    }

    void on_mouse_click(int x, int y) {
        if (!physics_service_ || !camera) return;

        // Convert screen coordinates to a world-space ray
        Vec3 origin, direction;
        camera->screen_point_to_ray(x, y, origin, direction);

        auto result = physics_service_->ray_cast(origin, direction, 1000.0f);

        if (result) {
            select_object(result->other_body);
        }
    }

    void select_object(PhysicsBody* body) {
        S_INFO("Selected physics body");
        // Highlight the selected object, etc.
    }
};
```

---

## 4. Contact Filtering with Raycasts

Raycasts respect the `ContactFilter` set on the `PhysicsService`. If your filter returns `false` from `should_collide()` for a pair of fixtures, the ray will pass through those fixtures as if they don't exist.

### 4.1 Filtering by Kind

```cpp
class RaycastContactFilter : public ContactFilter {
public:
    bool should_collide(const Fixture* lhs, const Fixture* rhs) const override {
        // Only collide with specific kinds
        uint16_t kind = lhs->kind();  // or rhs->kind()

        // Ignore sensor fixtures
        constexpr uint16_t SENSOR = 1 << 15;
        if (kind == SENSOR) return false;

        return true;
    }
};
```

Apply the filter:

```cpp
auto filter = std::make_unique<RaycastContactFilter>();
physics_service->set_contact_filter(filter.get());
```

### 4.2 Selective Ray Casting

By combining kind values with a contact filter, you can create selective raycasts:

```cpp
constexpr uint16_t TERRAIN = 1 << 0;
constexpr uint16_t ENEMIES = 1 << 1;
constexpr uint16_t PICKUPS = 1 << 2;
constexpr uint16_t TRIGGERS = 1 << 15;

// Terrain-only raycast filter
class TerrainFilter : public ContactFilter {
public:
    bool should_collide(const Fixture* lhs, const Fixture* rhs) const override {
        uint16_t kind = lhs->kind();
        if (kind == TERRAIN) return true;
        return false;
    }
};
```

---

## 5. The ContactFilter System

The `ContactFilter` class controls both collision detection and physical response between fixtures:

```cpp
class ContactFilter {
public:
    virtual bool should_collide(const Fixture* lhs, const Fixture* rhs) const = 0;

    virtual bool should_respond(const Fixture* lhs, const Fixture* rhs) const {
        return true;
    }
};
```

### 5.1 should_collide

Determines whether two fixtures should be tested for collision at all. Return `false` to make the fixtures completely ignore each other (both collision detection and physical response).

### 5.2 should_respond

Determines whether a detected collision should cause a physical response. Return `false` to detect the collision (events still fire) but not physically push the bodies apart. This is how sensors work.

```cpp
class GameContactFilter : public ContactFilter {
public:
    bool should_collide(const Fixture* lhs, const Fixture* rhs) const override {
        // Use kind values to define collision matrices
        uint16_t a = lhs->kind();
        uint16_t b = rhs->kind();

        // Players don't collide with each other
        if (a == PLAYER && b == PLAYER) return false;

        // Sensors collide with everything
        if (a == SENSOR || b == SENSOR) return true;

        return true;
    }

    bool should_respond(const Fixture* lhs, const Fixture* rhs) const override {
        // Sensors detect but don't respond physically
        if (lhs->kind() == SENSOR || rhs->kind() == SENSOR) {
            return false;
        }
        return true;
    }
};
```

---

## 6. Debug Visualization

The `PhysicsService` can be connected to a `Debug` node for visualizing the physics world. While this primarily shows colliders and body boundaries, it helps verify that raycasts are interacting with the correct geometry:

```cpp
auto debug_node = create_child<Debug>();
physics_service->set_debug(debug_node);
```

---

## 7. Performance Considerations

### 7.1 Cost of Raycasting

Raycasting is generally cheap, but the cost depends on:

- **Number of bodies** in the scene
- **Collider complexity** (mesh colliders are more expensive to test)
- **Ray distance** (longer rays may test against more objects)

### 7.2 Tips

1. **Use maximum distance** -- Don't cast rays further than necessary.
2. **Use simple colliders** for objects that are frequently raycast against.
3. **Filter with ContactFilter** to skip irrelevant fixtures.
4. **Cache results** if you're performing the same raycast every frame.

```cpp
// Good: Limited raycast distance
auto result = physics_service_->ray_cast(origin, direction, 50.0f);

// Bad: Unlimited raycast in a crowded scene
auto result = physics_service_->ray_cast(origin, direction);
```

---

## 8. Common Pitfalls

### 8.1 Ray Starts Inside a Collider

If the ray origin is inside a collider, the behavior may be unpredictable. Offset the origin slightly:

```cpp
// Bad: origin might be inside the muzzle collider
auto result = physics_service_->ray_cast(muzzle_pos, direction);

// Good: offset origin slightly along the direction
auto result = physics_service_->ray_cast(
    muzzle_pos + direction * 0.1f, direction, 100.0f
);
```

### 8.2 Direction Not Normalized

Always normalize your direction vector:

```cpp
// Bad
auto result = physics_service_->ray_cast(origin, direction, 100.0f);

// Good
auto result = physics_service_->ray_cast(origin, direction.normalized(), 100.0f);
```

### 8.3 Forgetting to Check the Result

`ray_cast` returns an `optional`. Always check if it has a value:

```cpp
auto result = physics_service_->ray_cast(origin, direction);

if (result) {
    // Safe to access result->
} else {
    // Hit nothing
}
```

### 8.4 PhysicsService Not Available

If the `PhysicsService` isn't found, `ray_cast` won't be available:

```cpp
physics_service_ = scene->find_service<PhysicsService>();
if (!physics_service_) {
    S_ERROR("No PhysicsService found in scene!");
    return;
}
```

---

## 9. Summary

| Feature | Details |
|---------|---------|
| Method | `PhysicsService::ray_cast(origin, direction, max_distance)` |
| Return type | `smlt::optional<RayCastResult>` |
| Result fields | `other_body`, `distance`, `normal`, `impact_point` |
| Filtering | Via `ContactFilter::should_collide()` |
| Max distance | Default is `std::numeric_limits<float>::max()` |

---

## See Also

- [Physics Overview](overview.md) -- General physics introduction
- [Rigid Bodies](rigid-bodies.md) -- Body types and properties
- [Colliders](colliders.md) -- Collider shapes and materials
- [Joints](joints.md) -- Connecting bodies together
- [Best Practices](best-practices.md) -- Optimization and patterns
