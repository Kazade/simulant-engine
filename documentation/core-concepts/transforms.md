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

### Position

```cpp
// Absolute position
node->move_to(x, y, z);
node->move_to(Vec3(x, y, z));

// Relative movement
node->move_by(dx, dy, dz);
node->move_by(Vec3(dx, dy, dz));

// Get position
Vec3 pos = node->position();          // Local position
Vec3 world_pos = node->absolute_position();  // World position
```

### Rotation

```cpp
// Set rotation (Euler angles)
node->rotate_to(pitch, yaw, roll);
node->rotate_to(Degrees(90), Degrees(45), Degrees(0));

// Relative rotation
node->rotate_by(Degrees(10), Degrees(0), Degrees(0));

// Get rotation
Euler rotation = node->euler_angles();
Quaternion quat = node->rotation();

// Rotate to face a point
node->look_at(target_position);
node->look_at(target_position, up_vector);
```

### Scale

```cpp
// Uniform scale
node->scale_by(2.0f);  // Double size

// Per-axis scale
node->set_scale(sx, sy, sz);
node->set_scale(Vec3(2.0f, 1.0f, 1.0f));  // Stretch on X

// Get scale
Vec3 scale = node->scale();
```

## Local vs World Space

### Local Space

Local transform is relative to the parent node:

```cpp
auto parent = create_child<Stage>();
parent->move_to(10, 0, 0);

auto child = parent->create_child<Actor>();
child->move_to(5, 0, 0);  // 5 units from parent

// Local position: (5, 0, 0)
Vec3 local = child->position();

// World position: (15, 0, 0)
Vec3 world = child->absolute_position();
```

### World Space

World transform is the absolute position in the scene:

```cpp
// Convert local to world
Vec3 world_pos = node->absolute_position();
Quaternion world_rot = node->absolute_rotation();
Vec3 world_scale = node->absolute_scale();
```

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

When you move a parent, all children move with it:

```cpp
auto car = create_child<Stage>();
car->move_to(0, 0, 0);

auto wheel_fl = car->create_child<Actor>();
wheel_fl->move_to(1, 0, 1);

auto wheel_fr = car->create_child<Actor>();
wheel_fr->move_to(-1, 0, 1);

// Move the car - wheels move with it
car->move_by(5, 0, 0);

// Wheels are now at (6, 0, 1) and (4, 0, 1) in world space
```

### Nested Hierarchies

Transforms compound through the hierarchy:

```cpp
auto grandparent = create_child<Stage>();
grandparent->move_to(10, 0, 0);

auto parent = grandparent->create_child<Stage>();
parent->move_to(5, 0, 0);

auto child = parent->create_child<Actor>();
child->move_to(2, 0, 0);

// Child's world position: 10 + 5 + 2 = 17
Vec3 world_pos = child->absolute_position();  // (17, 0, 0)
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
// Move forward
node->move_by(node->forward() * speed * dt);

// Strafe right
node->move_by(node->right() * speed * dt);
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
            
            self->move_to(
                target->absolute_position().x + x,
                target->absolute_position().y + 2,
                target->absolute_position().z + z
            );
            
            self->look_at(target->absolute_position());
        }
    }
};
```

### Follow at Offset

```cpp
// Camera follows player at offset
camera->move_to(
    player->absolute_position().x,
    player->absolute_position().y + 5,
    player->absolute_position().z - 10
);
camera->look_at(player->absolute_position());
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
Vec3 current_pos = node->position();
Vec3 new_pos = lerp(current_pos, target_pos, 0.1f);
node->move_to(new_pos);
```

### Clamp Position

```cpp
// Keep node within bounds
Vec3 pos = node->position();
pos.x = std::clamp(pos.x, -50.0f, 50.0f);
pos.z = std::clamp(pos.z, -50.0f, 50.0f);
node->move_to(pos);
```

## Debugging Transforms

Enable debug drawing to visualize transforms:

```cpp
// Draw axes at node position
stage->debug->draw_line(
    node->absolute_position(),
    node->absolute_position() + node->right() * 2,
    Colour::RED  // X axis
);
stage->debug->draw_line(
    node->absolute_position(),
    node->absolute_position() + node->up() * 2,
    Colour::GREEN  // Y axis
);
stage->debug->draw_line(
    node->absolute_position(),
    node->absolute_position() + node->forward() * 2,
    Colour::BLUE  // Z axis
);
```

## Best Practices

### 1. Use Parent-Child for Logical Grouping

```cpp
// Good: Car contains wheels
auto car = create_child<Stage>();
auto wheel = car->create_child<Actor>();

// Bad: Manually syncing positions
wheel->move_to(car->position() + offset);
```

### 2. Cache World Transforms When Needed

If you access world transforms frequently, cache them:

```cpp
// Bad: Recalculates every call
for (int i = 0; i < 100; ++i) {
    Vec3 pos = node->absolute_position();
}

// Good: Calculate once
Vec3 world_pos = node->absolute_position();
for (int i = 0; i < 100; ++i) {
    // Use world_pos
}
```

### 3. Use Local Space for Editing

When building scenes, work in local space when possible:

```cpp
// Position children relative to parent
wheel->move_to(1, 0, 1);  // Not absolute coordinates
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

Remember that transforms are hierarchical. Check parent transforms:

```cpp
for (auto ancestor : node->each_ancestor()) {
    S_DEBUG("Ancestor at {}", ancestor->position());
}
```

### Rotation Gimbal Lock

Use quaternions for complex rotations:

```cpp
// Avoid multiple Euler rotations
node->rotate_by(pitch, 0, 0);
node->rotate_by(0, yaw, 0);  // Can cause gimbal lock

// Use quaternion instead
Quaternion rot = node->rotation();
rot = rot * Quaternion::from_axis_angle(Vec3(0, 1, 0), yaw);
node->set_rotation(rot);
```

## See Also

- **[Stage Nodes](stage-nodes.md)** - Node hierarchy
- **[Actors](actors.md)** - Rendering meshes
- **[Cameras](cameras.md)** - Viewpoints
- **[Math Library](../utilities/math.md)** - Vec3, Quaternion, Euler
