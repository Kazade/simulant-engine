# Colliders

Colliders (also called fixtures) define the physical shape and surface properties of a physics body. They determine how bodies interact during collisions. This guide covers collider shapes, materials, collision filtering, and sensors.

**Related documentation:** [Physics Overview](overview.md), [Rigid Bodies](rigid-bodies.md), [Joints](joints.md).

---

## 1. What is a Collider?

A collider is a geometric shape attached to a `PhysicsBody`. The physics engine uses these shapes to detect and resolve collisions. A single body can have multiple colliders attached to it.

Each collider carries:

- A **shape** (box, sphere, capsule, triangle, or mesh)
- A **PhysicsMaterial** (density, friction, bounciness)
- A **kind** value (`uint16_t`) used for collision filtering
- A position and rotation relative to the body

---

## 2. Collider Shapes

### 2.1 Box Collider

The most versatile and performant collider. Boxes are defined by their half-extents (size along each axis from the center).

```cpp
PhysicsMaterial mat = PhysicsMaterial::wood();

// Simple box centered on the body
body->add_box_collider(Vec3(1, 0.5f, 1), mat);

// Box with offset (e.g., a collider above the body center)
body->add_box_collider(
    Vec3(1, 0.5f, 1),     // Half-extents
    mat,                   // Material
    0,                     // Kind (collision layer)
    Vec3(0, 1, 0),        // Offset from body center
    Quaternion()           // Rotation (none)
);

// Rotated box collider
body->add_box_collider(
    Vec3(1, 0.5f, 1),
    mat,
    0,
    Vec3(),
    Quaternion::from_axis_angle(Vec3(0, 1, 0), 45)
);
```

**Use for:** Crates, walls, platforms, most rectangular objects, character hitboxes.

### 2.2 Sphere Collider

Spheres are defined by their diameter. They are the cheapest collider to compute and are ideal for round objects.

```cpp
PhysicsMaterial mat = PhysicsMaterial::rubber();

// Sphere centered on the body
body->add_sphere_collider(1.0f, mat);  // 1.0 = diameter

// Offset sphere (e.g., a ball at the end of a chain)
body->add_sphere_collider(
    0.5f,                // Diameter
    mat,                  // Material
    0,                    // Kind
    Vec3(0, -2, 0)      // Offset
);
```

**Use for:** Balls, wheels, simple character bounds, pickup triggers.

### 2.3 Capsule Collider

Capsules are defined by two endpoints and a diameter. They are cylinders with hemispherical caps, ideal for character controllers.

```cpp
PhysicsMaterial mat = PhysicsMaterial::wood();

// Vertical capsule (typical for a standing character)
body->add_capsule_collider(
    Vec3(0, -0.5f, 0),  // Bottom endpoint
    Vec3(0, 0.5f, 0),   // Top endpoint
    0.3f,                // Diameter
    mat                   // Material
);

// Horizontal capsule
body->add_capsule_collider(
    Vec3(-0.5f, 0, 0),  // Left endpoint
    Vec3(0.5f, 0, 0),   // Right endpoint
    0.2f,                // Diameter
    mat
);
```

**Use for:** Characters, pills, elongated round objects.

### 2.4 Triangle Collider

A single triangle defined by three vertices. Useful for flat surfaces or as building blocks for more complex shapes.

```cpp
PhysicsMaterial mat = PhysicsMaterial::stone();

body->add_triangle_collider(
    Vec3(-1, 0, -1),  // Vertex 1
    Vec3(1, 0, -1),   // Vertex 2
    Vec3(0, 0, 1),    // Vertex 3
    mat,               // Material
    0                  // Kind
);
```

**Use for:** Ramps, inclined planes, simple terrain features.

### 2.5 Mesh Collider

Mesh colliders use the triangles from a `MeshPtr` to create complex collision geometry. Only available on `StaticBody`.

```cpp
auto mesh = assets->load_mesh("models/building.obj");

auto body = create_child<StaticBody>();
PhysicsMaterial mat = PhysicsMaterial::stone();
body->add_mesh_collider(mesh, mat);
```

You can also position, orient, and scale the mesh collider independently:

```cpp
body->add_mesh_collider(
    mesh,
    mat,
    0,                        // Kind (collision layer)
    Vec3(0, 5, 0),           // Position offset
    Quaternion(),             // Orientation
    Vec3(2, 1, 2)            // Scale
);
```

**Use for:** Complex static geometry like buildings, terrain, sculptures.

**Warning:** Mesh colliders are significantly more expensive than primitive shapes. Use them sparingly and only for static geometry.

---

## 3. Physics Materials

Materials define the surface properties of a collider. They control how the collider interacts physically with other colliders.

### 3.1 Built-in Materials

```cpp
PhysicsMaterial wood = PhysicsMaterial::wood();
PhysicsMaterial rubber = PhysicsMaterial::rubber();
PhysicsMaterial iron = PhysicsMaterial::iron();
PhysicsMaterial stone = PhysicsMaterial::stone();
```

| Material | Density | Friction | Bounciness | Typical Use |
|----------|---------|----------|-------------|-------------|
| Wood | 0.005 | 0.4 | 0.2 | Crates, wooden platforms |
| Rubber | 0.001 | 0.3 | 0.8 | Balls, bumpers |
| Iron | 0.1 | 0.2 | ~0 | Heavy machinery, metal objects |
| Stone | 0.1 | 0.8 | ~0 | Ground, walls, terrain |

### 3.2 Custom Materials

```cpp
// density, friction, bounciness
PhysicsMaterial ice(0.001f, 0.0f, 0.1f);       // Slippery
PhysicsMaterial superball(0.001f, 0.1f, 0.95f); // Very bouncy
PhysicsMaterial sandpaper(0.005f, 1.0f, 0.0f);  // Very grippy
```

### 3.3 Density Multipliers

Built-in materials accept an optional density multiplier:

```cpp
PhysicsMaterial light_wood = PhysicsMaterial::wood(0.5f);   // Half density
PhysicsMaterial heavy_iron = PhysicsMaterial::iron(5.0f);   // 5x density
```

### 3.4 Material Property Reference

| Property | Description | Range | Effect |
|----------|-------------|-------|--------|
| `density` | Mass per unit volume | 0.0+ | Higher = heavier body |
| `friction` | Surface grip | 0.0--1.0 | 0 = ice, 1 = sandpaper |
| `bounciness` | Restitution coefficient | 0.0--1.0 | 0 = dead, 1 = perfectly elastic |

---

## 4. Collision Filtering

The `kind` parameter on every collider is a `uint16_t` value used with a `ContactFilter` to control which colliders can collide with each other.

### 4.1 Creating a Contact Filter

```cpp
class GameContactFilter : public ContactFilter {
public:
    bool should_collide(const Fixture* lhs, const Fixture* rhs) const override {
        uint16_t kind_a = lhs->kind();
        uint16_t kind_b = rhs->kind();

        // Define collision categories using bit flags
        constexpr uint16_t PLAYER = 1 << 0;
        constexpr uint16_t ENEMY = 1 << 1;
        constexpr uint16_t BULLET = 1 << 2;
        constexpr uint16_t PICKUP = 1 << 3;

        // Players don't collide with other players
        if (kind_a == PLAYER && kind_b == PLAYER) return false;

        // Enemies don't collide with other enemies
        if (kind_a == ENEMY && kind_b == ENEMY) return false;

        // Bullets hit everything except pickups
        if (kind_a == BULLET && kind_b == PICKUP) return false;
        if (kind_b == BULLET && kind_a == PICKUP) return false;

        return true;
    }

    bool should_respond(const Fixture* lhs, const Fixture* rhs) const override {
        // Return false to detect collision but not physically respond
        // (useful for trigger zones that still report contact events)
        return true;
    }
};
```

### 4.2 Applying the Filter

```cpp
auto filter = std::make_unique<GameContactFilter>();
physics_service->set_contact_filter(filter.get());

// Keep the filter alive for as long as the simulation runs
// (the service holds a weak reference)
```

### 4.3 Using Kind Values

When creating colliders, assign kind values that match your filter logic:

```cpp
constexpr uint16_t PLAYER = 1 << 0;
constexpr uint16_t ENEMY = 1 << 1;

// Player collider
player_body->add_capsule_collider(
    Vec3(0, -0.5f, 0), Vec3(0, 0.5f, 0), 0.3f,
    PhysicsMaterial::wood(), PLAYER
);

// Enemy collider
enemy_body->add_box_collider(
    Vec3(0.5f, 1, 0.5f),
    PhysicsMaterial::wood(), ENEMY
);
```

---

## 5. Sensors

Sensors are colliders that detect overlaps without causing a physical response. Objects pass through sensors but collision events are still fired.

### 5.1 Creating Sensors

The physics system uses `ContactFilter::should_respond()` to determine whether a collision should be physical or sensor-only. Return `false` from `should_respond()` for sensor behavior:

```cpp
class SensorContactFilter : public ContactFilter {
public:
    bool should_collide(const Fixture* lhs, const Fixture* rhs) const override {
        // Sensors use a specific kind value
        constexpr uint16_t SENSOR = 1 << 15;
        if (lhs->kind() == SENSOR || rhs->kind() == SENSOR) {
            return true;  // Detect the overlap
        }
        return true;
    }

    bool should_respond(const Fixture* lhs, const Fixture* rhs) const override {
        constexpr uint16_t SENSOR = 1 << 15;
        // If either fixture is a sensor, don't physically respond
        if (lhs->kind() == SENSOR || rhs->kind() == SENSOR) {
            return false;
        }
        return true;
    }
};
```

### 5.2 Sensor Use Cases

```cpp
constexpr uint16_t SENSOR = 1 << 15;

// Pickup trigger zone
auto pickup = create_child<Actor>();
auto body = pickup->create_child<StaticBody>();
body->add_sphere_collider(1.0f, PhysicsMaterial(), SENSOR);

body->signal_collision_enter().connect([](const Collision& collision) {
    S_INFO("Player entered pickup zone!");
    // Trigger pickup logic
});

// Door trigger zone
auto door_trigger = create_child<Actor>();
auto trigger_body = door_trigger->create_child<StaticBody>();
trigger_body->add_box_collider(Vec3(1, 2, 0.5f), PhysicsMaterial(), SENSOR);

trigger_body->signal_collision_enter().connect([](const Collision& collision) {
    open_door();
});

trigger_body->signal_collision_exit().connect([](const Collision& collision) {
    close_door();
});
```

---

## 6. Multiple Fixtures per Body

A single physics body can have multiple colliders. This is useful for complex shapes or for combining physical colliders with sensors.

### 6.1 Compound Colliders

```cpp
auto body = actor->create_child<DynamicBody>();
PhysicsMaterial mat = PhysicsMaterial::wood();

// L-shaped object made of two boxes
body->add_box_collider(Vec3(1, 0.2f, 0.2f), mat, 0, Vec3(0, 0, 0));
body->add_box_collider(Vec3(0.2f, 1, 0.2f), mat, 0, Vec3(-0.8f, 0.8f, 0));
```

### 6.2 Physical Collider + Sensor

```cpp
auto body = actor->create_child<DynamicBody>();

// Main physical collider
body->add_box_collider(Vec3(0.5f, 1, 0.5f), PhysicsMaterial::wood());

// Ground detection sensor at the feet
constexpr uint16_t SENSOR = 1 << 15;
body->add_box_collider(
    Vec3(0.4f, 0.05f, 0.4f),
    PhysicsMaterial(),    // Material doesn't matter for sensors
    SENSOR,
    Vec3(0, -1.05f, 0)   // Just below the feet
);
```

Check ground contact via the contact list:

```cpp
bool is_grounded() {
    for (auto it = body->contacts().begin(); it != body->contacts().end(); ++it) {
        Contact contact = *it;
        // Check if the contact involves the sensor fixture
        // (you'd check fixture kind here)
    }
    return false;
}
```

---

## 7. The Fixture Class

Internally, colliders are represented by the `Fixture` class. You generally don't create fixtures directly -- they are created through the body's `add_*_collider` methods. The `Fixture` class provides:

```cpp
class Fixture {
public:
    const PhysicsBody* body() const;  // Get the body this fixture belongs to
    uint16_t kind() const;             // Get the kind value
};
```

Fixtures are referenced in collision callbacks and through the `ContactList`:

```cpp
struct Contact {
    Contact(const Fixture& a, const Fixture& b) : fixtures{a, b} {}
    Fixture fixtures[2];
};
```

---

## 8. Collider Performance

Collider choice has a significant impact on physics performance. Here is the approximate cost from cheapest to most expensive:

| Shape | Relative Cost | Notes |
|-------|--------------|-------|
| Sphere | Lowest | Single radius check, very fast |
| Capsule | Low | Two sphere + cylinder tests |
| Box | Low-Medium | SAT (Separating Axis Theorem) |
| Triangle | Medium | Plane and edge tests |
| Mesh | Highest | Triangle soup, many tests |

### Performance Tips

1. **Prefer primitive shapes** over mesh colliders whenever possible.
2. **Use the simplest shape** that accurately represents your object.
3. **Combine multiple simple colliders** rather than using a single mesh collider.
4. **Avoid mesh colliders on dynamic bodies** -- they are only available on `StaticBody` for a reason.
5. **Use sphere colliders for triggers** when the exact shape doesn't matter.

---

## 9. Common Patterns

### 9.1 Character Controller Setup

```cpp
class CharacterBehaviour : public StageNode {
public:
    FindResult<DynamicBody> body = smlt::FindDescendent<DynamicBody>(this);

    void on_load() override {
        PhysicsMaterial mat = PhysicsMaterial::wood();

        // Body collider (capsule)
        body->add_capsule_collider(
            Vec3(0, -0.4f, 0),
            Vec3(0, 0.6f, 0),
            0.3f,
            mat
        );

        // Ground sensor
        constexpr uint16_t SENSOR = 1 << 15;
        body->add_sphere_collider(0.1f, PhysicsMaterial(), SENSOR, Vec3(0, -0.55f, 0));
    }

    bool is_grounded() const {
        // Check contacts for ground sensor
        for (auto it = body->contacts().begin(); it != body->contacts().end(); ++it) {
            // Check if contact normal points upward
            // (simplified -- you'd check the specific fixture)
        }
        return false;
    }
};
```

### 9.2 Vehicle with Wheel Sensors

```cpp
auto chassis = create_child<DynamicBody>();
chassis->add_box_collider(Vec3(1, 0.3f, 0.5f), PhysicsMaterial::iron());

// Wheel contact sensors (for suspension/ground detection)
constexpr uint16_t WHEEL_SENSOR = 1 << 5;
chassis->add_sphere_collider(0.1f, PhysicsMaterial(), WHEEL_SENSOR, Vec3(-0.8f, -0.5f, 0.4f));
chassis->add_sphere_collider(0.1f, PhysicsMaterial(), WHEEL_SENSOR, Vec3(-0.8f, -0.5f, -0.4f));
chassis->add_sphere_collider(0.1f, PhysicsMaterial(), WHEEL_SENSOR, Vec3(0.8f, -0.5f, 0.4f));
chassis->add_sphere_collider(0.1f, PhysicsMaterial(), WHEEL_SENSOR, Vec3(0.8f, -0.5f, -0.4f));
```

### 9.3 Room with Mesh Walls

```cpp
auto room_mesh = assets->load_mesh("models/room.obj");
auto room = create_child<StaticBody>();
room->add_mesh_collider(room_mesh, PhysicsMaterial::stone());
```

---

## See Also

- [Physics Overview](overview.md) -- General physics introduction
- [Rigid Bodies](rigid-bodies.md) -- Body types and properties
- [Joints](joints.md) -- Connecting bodies together
- [Raycasting](raycasting.md) -- Querying the physics world
- [Best Practices](best-practices.md) -- Optimization and patterns
