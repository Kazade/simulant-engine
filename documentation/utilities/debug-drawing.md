# Debug Drawing

Source: `simulant/nodes/debug.h`

Every `Stage` exposes a `debug` property of type `Debug*` that lets you render temporary lines, rays, and points directly in the scene. This is useful for visualizing physics shapes, raycasts, bounding volumes, AI paths, and any other runtime geometry without creating permanent mesh assets.

---

## Basic Usage

```cpp
#include <simulant/nodes/debug.h>

// Draw a red line from the origin to (1, 1, 1)
stage->debug->draw_line(
    Vec3(0, 0, 0),           // start
    Vec3(1, 1, 1),           // end
    Color::red(),            // color
    Seconds(),               // duration (0 = one frame)
    false                    // depth test
);

// Draw a ray (start + direction)
stage->debug->draw_ray(
    Vec3(0, 0, 0),           // origin
    Vec3(0, 1, 0),           // direction
    Color::white(),          // default color
    Seconds(),               // default: one frame
    false                    // no depth test
);

// Draw a blue point
stage->debug->draw_point(
    Vec3(5, 3, 2),           // position
    Color::blue(),           // color
    Seconds(1.0f),           // lasts 1 second
    false                    // visible through geometry
);
```

All three drawing functions share the same parameter pattern:

| Parameter | Type | Default | Description |
|---|---|---|---|
| `start` / `position` | `Vec3` | (required) | World-space start point |
| `end` / `dir` | `Vec3` | (required) | World-space end point (line) or direction vector (ray) |
| `color` | `Color` | `Color::white()` | Line / point color |
| `duration` | `Seconds` | `Seconds()` (one frame) | How long the element persists |
| `depth_test` | `bool` | `false` | Whether the element is occluded by scene geometry |

---

## Duration and Lifecycle

The `duration` parameter controls how long a debug element persists on screen:

- **`Seconds()` (default)** -- the element is drawn for one frame only. Call every frame to keep it visible.
- **`Seconds(1.0f)`** -- the element persists for 1 second automatically, then fades out.
- **`Seconds(5.0f)`** -- persists for 5 seconds.

This is useful for drawing one-shot debug visuals (e.g., collision impact points) without managing their lifecycle manually.

```cpp
// One-frame: must be called every update
stage->debug->draw_line(a, b, Color::green());

// Persists for 2 seconds, no further calls needed
stage->debug->draw_point(impact_point, Color::red(), Seconds(2.0f));
```

---

## Depth Testing

By default, debug elements are drawn **without** depth testing so they are always visible on top of scene geometry. Pass `true` for the `depth_test` parameter to make them respect the depth buffer:

```cpp
// Always visible (default)
stage->debug->draw_line(start, end, Color::yellow());

// Occluded by walls, floors, etc.
stage->debug->draw_line(start, end, Color::yellow(), Seconds(), true);
```

---

## Configuration

Adjust the visual size of lines and points globally:

```cpp
// Adjust visual size
stage->debug->set_point_size(0.05f);
stage->debug->set_line_width(0.02f);

float ps = stage->debug->point_size();
float lw = stage->debug->line_width();
```

These settings affect all subsequently drawn elements. Points are rendered as small axis-aligned quads (since many platforms do not support native point sprite rendering).

---

## Practical Examples

### Visualize a Velocity Vector

```cpp
stage->debug->draw_ray(
    entity->translation(),
    entity->velocity().normalized() * 2.0f,
    Color::green()
);
```

### Draw a Bounding Box Wireframe

The `Debug` class does not have a built-in `draw_box` function, but you can easily draw an AABB wireframe by connecting its corners with `draw_line`:

```cpp
void draw_aabb(Debug* debug, const AABB& aabb, const Color& color,
               Seconds duration = Seconds(), bool depth_test = false) {
    auto corners = aabb.corners();

    // Bottom face
    debug->draw_line(corners[0], corners[1], color, duration, depth_test);
    debug->draw_line(corners[1], corners[2], color, duration, depth_test);
    debug->draw_line(corners[2], corners[3], color, duration, depth_test);
    debug->draw_line(corners[3], corners[0], color, duration, depth_test);

    // Top face
    debug->draw_line(corners[4], corners[5], color, duration, depth_test);
    debug->draw_line(corners[5], corners[6], color, duration, depth_test);
    debug->draw_line(corners[6], corners[7], color, duration, depth_test);
    debug->draw_line(corners[7], corners[4], color, duration, depth_test);

    // Vertical edges
    debug->draw_line(corners[0], corners[4], color, duration, depth_test);
    debug->draw_line(corners[1], corners[5], color, duration, depth_test);
    debug->draw_line(corners[2], corners[6], color, duration, depth_test);
    debug->draw_line(corners[3], corners[7], color, duration, depth_test);
}
```

### Mark a Collision Point

```cpp
stage->debug->draw_point(collision_point, Color::red(), Seconds(0.5f));
```

### Visualize a Raycast

```cpp
auto result = physics->ray_cast(origin, direction, 100.0f);

// Draw the ray
stage->debug->draw_ray(origin, direction * 100.0f, Color::cyan());

// Mark the hit point if one was found
if (result) {
    stage->debug->draw_point(result->impact_point, Color::red(), Seconds(1.0f));
}
```

### Draw a Skeleton Hierarchy

```cpp
// Assuming you have access to joint absolute positions
for (auto& joint : skeleton->joints()) {
    if (joint->parent()) {
        debug->draw_line(
            joint->parent()->absolute_translation(),
            joint->absolute_translation(),
            Color::yellow()
        );
    }
}
```

---

## Colors

Simulant provides the `Color` struct with several static factory methods for common colors:

```cpp
#include <simulant/color.h>

Color::white();
Color::black();
Color::red();
Color::green();
Color::blue();
Color::yellow();
Color::purple();
Color::turquoise();
Color::gray();
Color::none();  // fully transparent

// Custom color (RGBA, 0.0 - 1.0 range)
Color custom(1.0f, 0.5f, 0.0f, 1.0f);

// From 8-bit bytes (0 - 255 range)
Color from_bytes = Color::from_bytes(255, 128, 0, 255);
```

---

## Performance Considerations

- Debug drawing is intentionally lightweight. Elements are batched into a single mesh each frame and rendered with a built-in unlit material.
- Debug elements are drawn at the absolute foreground render priority, so they appear on top of all scene geometry (unless `depth_test` is `true`).
- Depth writing is always disabled for debug rendering, so debug elements never occlude other geometry.
- Because elements with a non-zero `duration` are stored internally and updated each frame, avoid creating a large number of long-lived elements. Use one-frame duration and redraw explicitly when you need dense, persistent visuals.
- Calling `draw_ray` is a thin wrapper around `draw_line` -- it simply draws from `start` to `start + dir`.

---

## Physics Debug Integration

The engine's built-in physics service (`PhysicsService`) uses the `Debug` node to render collision shapes when a debug overlay is active. The physics `DebugDraw` implementation calls `draw_line` and `draw_point` under the hood with per-frame duration:

```cpp
// Inside the physics service update (simplified)
if (debug_) {
    DebugDraw draw(debug_);
    for (b3Body* b = scene_->GetBodyList(); b; b = b->GetNext()) {
        for (b3Fixture* f = b->GetFixtureList(); f; f = f->GetNext()) {
            f->Draw(&draw, b3Color(1, 1, 1, 1));
        }
    }
}
```

This means that simply connecting a `Debug` node to your physics service will automatically render body shapes, joints, and contact points every frame without any additional code.
