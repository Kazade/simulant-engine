# Transforms & Hierarchy

Understanding how positioning, rotation, and scaling work in Simulant is fundamental to building games. This guide covers the transform system and how nodes relate to each other spatially.

## Overview

Every `StageNode` has a **transform** that defines its position, rotation, and scale in 3D space. Transforms are hierarchical - a node's final (world) transform is calculated by combining its own transform with all its parent transforms.

## The Transform Components

A transform consists of:

| Component | Type | Description |
|-----------|------|-------------|
| **Position** | `Vec3` | Location in 3D space (x, y, z) |
| **Rotation** | `Quaternion` / `Euler` | Orientation (pitch, yaw, roll) |
| **Scale** | `Vec3` | Size multiplier per axis |

## Coordinate System

Simulant uses a **right-handed coordinate system**:

```
     Y
     |
     |
     |____ X
    /
   /
  Z
```

- **+X**: Right
- **+Y**: Up  
- **+Z**: Toward the viewer (out of the screen)

Angles use **degrees** in Simulant, wrapped with the `Degrees()` helper:

```cpp
node->rotate_to(Degrees(90), Degrees(0), Degrees(0));  // 90° around X axis
```

## Setting Transforms

> **Important:** Simulant distinguishes between **absolute** and **relative** transform properties on the `Transform` class (accessed via `node->transform->`):
>
> | Property | Space | Description |
> |----------|-------|-------------|
> | `position` | **Absolute** (world-space) | Final world-space position |
> | `orientation` | **Absolute** (world-space) | Final world-space rotation as quaternion |
> | `scale` | **Absolute** (world-space) | Final world-space scale |
> | `translation` | **Relative** (local-space) | Offset from parent |
> | `rotation` | **Relative** (local-space) | Rotation relative to parent |
> | `scale_factor` | **Relative** (local-space) | Scale relative to parent |

### Position

```cpp
// Read ABSOLUTE world-space position
Vec3 world_pos = node->transform->position();

// Read RELATIVE local-space translation (offset from parent)
Vec3 local_offset = node->transform->translation();

// Set absolute world-space position
node->transform->set_position(Vec3(10, 5, 0));

// Set relative translation (offset from parent)
node->transform->set_translation(Vec3(2, 0, 0));  // 2 units right of parent
```

### Rotation

```cpp
// Read ABSOLUTE world-space orientation (quaternion)
Quaternion world_rot = node->transform->orientation();

// Read RELATIVE local-space rotation (quaternion)
Quaternion local_rot = node->transform->rotation();

// Set absolute world-space orientation
node->transform->set_orientation(Quaternion::from_euler(Degrees(90), Degrees(0), Degrees(0)));

// Set relative rotation (local to parent)
node->transform->set_rotation(Quaternion::from_euler(Degrees(45), Degrees(0), Degrees(0)));
```

### Scale

```cpp
// Read ABSOLUTE world-space scale
Vec3 world_scale = node->transform->scale();

// Read RELATIVE local-space scale factor
Vec3 local_scale = node->transform->scale_factor();

// Set absolute world-space scale
node->transform->set_scale(Vec3(2.0f, 2.0f, 2.0f));

// Set relative scale factor
node->transform->set_scale_factor(Vec3(2.0f, 1.0f, 1.0f));  // Stretch on X
```

### Relative Movement Helpers

```cpp
// Translate relative to current position (adds to translation)
node->transform->translate(Vec3(dx, dy, dz));

// Rotate relative to current orientation (adds to rotation)
node->transform->rotate(Vec3(0, 1, 0), Degrees(45));  // 45° around Y axis
node->transform->rotate(Degrees(10), Degrees(0), Degrees(0));  // Euler angles

// Scale relative to current scale
node->transform->scale_by(2.0f);  // Double current scale
node->transform->scale_by(Vec3(1.5f, 1.5f, 1.5f));
```

## Local vs World Space

### Understanding the Difference

```cpp
// position and orientation are ALWAYS absolute (world-space)
Vec3 world_pos = node->transform->position();
Quaternion world_rot = node->transform->orientation();

// translation and rotation are ALWAYS relative (local-space)
Vec3 local_offset = node->transform->translation();
Quaternion local_rot = node->transform->rotation();
```

### Example: Parent-Child Hierarchy

```cpp
auto parent = create_child<Stage>();
parent->transform->set_position(Vec3(10, 0, 0));  // Absolute world position

auto child = parent->create_child<Actor>();
child->transform->set_translation(Vec3(5, 0, 0));  // 5 units relative to parent

// Child's RELATIVE position (offset from parent): (5, 0, 0)
Vec3 local = child->transform->translation();

// Child's ABSOLUTE position (world-space): (15, 0, 0)
Vec3 world = child->transform->position();
```

> **Key Insight:** When you change `translation`, you're setting the offset from the parent. When you read `position`, you're getting the final world-space location after all parent transforms are applied.

## Transform Retain Mode

When reparenting a node, you can control whether it keeps its world transform:

```cpp
// Lose world transform (default) - adopts parent's local space
child->set_parent(new_parent, TRANSFORM_RETAIN_MODE_LOSE);

// Keep world transform - adjusts local to maintain world position
child->set_parent(new_parent, TRANSFORM_RETAIN_MODE_KEEP);
```

Example:

```cpp
auto actor = create_child<Actor>();
actor->move_to(10, 0, 0);

auto new_parent = create_child<Stage>();
new_parent->move_to(5, 0, 0);

// Without retain: actor moves to (5, 0, 0) in world space
actor->set_parent(new_parent, TRANSFORM_RETAIN_MODE_LOSE);

// With retain: actor stays at (10, 0, 0) in world space, local becomes (5, 0, 0)
actor->set_parent(new_parent, TRANSFORM_RETAIN_MODE_KEEP);
```

## Hierarchical Transforms

### Parent-Child Relationships

When you change a parent's absolute position, all children move with it:

```cpp
auto car = create_child<Stage>();
car->transform->set_position(Vec3(0, 0, 0));

auto wheel_fl = car->create_child<Actor>();
wheel_fl->transform->set_translation(Vec3(1, 0, 1));

auto wheel_fr = car->create_child<Actor>();
wheel_fr->transform->set_translation(Vec3(-1, 0, 1));

// Move the car in world space - wheels move with it
car->transform->set_position(Vec3(5, 0, 0));

// Wheels' world positions are now (6, 0, 1) and (4, 0, 1)
// But their translation values remain (1, 0, 1) and (-1, 0, 1)
```

### Nested Hierarchies

Transforms compound through the hierarchy:

```cpp
auto grandparent = create_child<Stage>();
grandparent->transform->set_position(Vec3(10, 0, 0));

auto parent = grandparent->create_child<Stage>();
parent->transform->set_translation(Vec3(5, 0, 0));  // Relative to grandparent

auto child = parent->create_child<Actor>();
child->transform->set_translation(Vec3(2, 0, 0));  // Relative to parent

// Child's ABSOLUTE position: 10 + 5 + 2 = 17
Vec3 world_pos = child->transform->position();  // (17, 0, 0)

// Child's RELATIVE translation is still just (2, 0, 0)
Vec3 local_offset = child->transform->translation();  // (2, 0, 0)
```

## Direction Vectors

Get direction vectors based on current rotation:

```cpp
// Forward direction (where the node is facing)
Vec3 forward = node->forward();

// Right direction
Vec3 right = node->right();

// Up direction
Vec3 up = node->up();
```

These are useful for movement:

```cpp
// Move forward (modify absolute position)
node->transform->translate(node->transform->forward() * speed * dt);

// Strafe right
node->transform->translate(node->transform->right() * speed * dt);
```

## Common Transform Patterns

### Orbit Around a Point

```cpp
class OrbitBehaviour : public StageNode {
public:
    FindResult<Actor> target = FindDescendent("Target", this);
    float radius = 5.0f;
    float angle = 0.0f;
    float speed = 90.0f;  // degrees per second

    void on_update(float dt) override {
        angle += speed * dt;

        if (auto self = maybe_this()) {
            float rad = angle * 3.14159f / 180.0f;
            float x = cos(rad) * radius;
            float z = sin(rad) * radius;

            // Set absolute position based on target's world position
            self->transform->set_position(Vec3(
                target->transform->position().x + x,
                target->transform->position().y + 2,
                target->transform->position().z + z
            ));

            self->transform->look_at(target->transform->position());
        }
    }
};
```

### Follow at Offset

```cpp
// Camera follows player at offset (modify absolute position)
camera->transform->set_position(Vec3(
    player->transform->position().x,
    player->transform->position().y + 5,
    player->transform->position().z - 10
));
camera->transform->look_at(player->transform->position());
```

### billboarding (Face Camera)

Use built-in billboards:

```cpp
// Always faces the camera
auto billboard = create_child<SphericalBillboard>();

// Or cylindrical (only rotates on Y axis)
auto cyl_billboard = create_child<CylindricalBillboard>();
```

## Transform Utilities

### Distance Calculation

```cpp
// Distance between two nodes
float dist = node_a->distance_to(node_b);

// Squared distance (faster, no sqrt)
float dist_sq = node_a->distance_squared_to(node_b);
```

### Lerp (Interpolation)

```cpp
// Smooth movement
Vec3 target_pos = Vec3(10, 0, 0);
Vec3 current_pos = node->transform->position();  // Read absolute position
Vec3 new_pos = lerp(current_pos, target_pos, 0.1f);
node->transform->set_position(new_pos);  // Set absolute position
```

### Clamp Position

```cpp
// Keep node within bounds
Vec3 pos = node->transform->position();  // Read absolute position
pos.x = std::clamp(pos.x, -50.0f, 50.0f);
pos.z = std::clamp(pos.z, -50.0f, 50.0f);
node->transform->set_position(pos);  // Set absolute position
```

## Debugging Transforms

Enable debug drawing to visualize transforms:

```cpp
// Draw axes at node position (use absolute position for world-space drawing)
stage->debug->draw_line(
    node->transform->position(),
    node->transform->position() + node->transform->right() * 2,
    Colour::RED  // X axis
);
stage->debug->draw_line(
    node->transform->position(),
    node->transform->position() + node->transform->up() * 2,
    Colour::GREEN  // Y axis
);
stage->debug->draw_line(
    node->transform->position(),
    node->transform->position() + node->transform->forward() * 2,
    Colour::BLUE  // Z axis
);
```

## Best Practices

### 1. Use Parent-Child for Logical Grouping

```cpp
// Good: Car contains wheels
auto car = create_child<Stage>();
auto wheel = car->create_child<Actor>();
wheel->transform->set_translation(Vec3(1, 0, 1));  // Relative to car

// Bad: Manually syncing positions
wheel->transform->set_position(car->transform->position() + offset);  // Don't do this!
```

### 2. Cache World Transforms When Needed

If you access world transforms frequently, cache them:

```cpp
// Bad: Accessing position repeatedly
for (int i = 0; i < 100; ++i) {
    Vec3 pos = node->transform->position();
}

// Good: Calculate once
Vec3 world_pos = node->transform->position();
for (int i = 0; i < 100; ++i) {
    // Use world_pos
}
```

> **Note:** Simulant's `position()` and `orientation()` compute the world transform from the hierarchy, so caching is beneficial in tight loops.

### 3. Use Local Space for Scene Building

When building scenes, use `translation` for positioning children relative to parents:

```cpp
// Good: Position children relative to parent
wheel->transform->set_translation(Vec3(1, 0, 1));  // 1 unit right, 1 unit forward of car

// Confusing: Using absolute positions for children
wheel->transform->set_position(Vec3(100, 0, 100));  // Hard to reason about!
```

### 4. Avoid Deep Hierarchies

Deep hierarchies impact performance:

```
// Bad: Too deep
root -> world -> chunk -> building -> room -> furniture -> detail

// Good: Flatter
root -> world
world -> buildings
buildings -> furniture
```

### 5. Use Finders for References

Don't hardcode paths through the hierarchy:

```cpp
// Bad: Fragile
auto wheel = find_descendent("car/body/suspension/wheel_fl");

// Good: Resilient
FindResult<Actor> wheel = FindDescendent("wheel_fl", this);
```

## Common Issues

### Node Not Moving

Check if the node's Stage is part of an active pipeline:

```cpp
if (!node->is_part_of_active_pipeline()) {
    S_WARN("Node won't be updated!");
}
```

### Unexpected Position

Remember that `position()` is absolute and hierarchical. A node's final position depends on all parent transforms:

```cpp
// Debug: Check ancestor positions
for (auto ancestor : node->each_ancestor()) {
    S_DEBUG("Ancestor at {}", ancestor->transform->position());
}

// Check the relative offset
S_DEBUG("Local translation: {}", node->transform->translation());
S_DEBUG("Absolute position: {}", node->transform->position());
```

### Rotation Gimbal Lock

Use quaternions for complex rotations:

```cpp
// Avoid multiple Euler rotations
node->transform->set_rotation(Quaternion::from_euler(Degrees(pitch), Degrees(0), Degrees(0)));
node->transform->set_rotation(Quaternion::from_euler(Degrees(0), Degrees(yaw), Degrees(0)));  // Can cause gimbal lock

// Use quaternion instead
Quaternion rot = node->transform->orientation();
rot = rot * Quaternion::from_axis_angle(Vec3(0, 1, 0), Degrees(yaw));
node->transform->set_orientation(rot);
```

## See Also

- **[Stage Nodes](stage-nodes.md)** - Node hierarchy
- **[Actors](actors.md)** - Rendering meshes
- **[Cameras](cameras.md)** - Viewpoints
- **[Math Library](../utilities/math.md)** - Vec3, Quaternion, Euler
