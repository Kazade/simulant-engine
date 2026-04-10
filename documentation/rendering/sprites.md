# 2D Game Development with Sprites

This guide covers 2D game development in Simulant, from rendering a single sprite to building a complete 2D platformer. Sprites are the foundation of 2D rendering in Simulant -- they are lightweight, animated, texture-mapped quads that integrate seamlessly with the engine's scene graph.

**Related documentation:** [Stage Nodes](../core-concepts/stage-nodes.md), [Animation](../animation/overview.md), [Cameras](../core-concepts/cameras.md), [Input](../input/overview.md), [Physics](../physics/overview.md).

---

## Table of Contents

1. [Overview of 2D Rendering in Simulant](#1-overview-of-2d-rendering-in-simulant)
2. [The Sprite Node](#2-the-sprite-node)
3. [Sprite Sheets and Configuration](#3-sprite-sheets-and-configuration)
4. [Creating Sprites from Textures](#4-creating-sprites-from-textures)
5. [Sprite Animations (Frame-by-Frame)](#5-sprite-animations-frame-by-frame)
6. [Animation States and Playback](#6-animation-states-and-playback)
7. [Camera2D for 2D Games](#7-camera2d-for-2d-games)
8. [Orthographic Projection](#8-orthographic-projection)
9. [2D Input Handling](#9-2d-input-handling)
10. [Tile-Based Rendering](#10-tile-based-rendering)
11. [2D Physics](#11-2d-physics)
12. [Complete 2D Game Example](#12-complete-2d-game-example--simple-platformer)
13. [Best Practices for 2D Games](#13-best-practices-for-2d-games)
14. [Performance Considerations](#14-performance-considerations)

---

## 1. Overview of 2D Rendering in Simulant

Simulant is a 3D-first engine, but it provides excellent 2D rendering capabilities through a combination of systems:

| System | Purpose |
|--------|---------|
| **Sprite** | 2D textured quad with sprite sheet animation support |
| **Camera2D** | Camera specialized for 2D/pixel-aligned rendering |
| **Orthographic projection** | Parallel projection that eliminates perspective distortion |
| **Render priority** | Controls draw order (background to foreground) |
| **Alpha blending** | Transparency for layered 2D visuals |

### How 2D Rendering Works

In Simulant, 2D rendering follows the same pipeline as 3D rendering, but with key differences:

1. **Camera2D** uses an orthographic projection matrix instead of perspective. This means objects do not get smaller with distance -- a sprite at Z=0 and a sprite at Z=5 appear the same size.
2. **Sprite** nodes are internally a simple quad mesh (4 vertices) with a material that has alpha blending enabled. The texture coordinates are updated each frame to display the correct frame from a sprite sheet.
3. **Render priority** determines draw order. Sprites with lower priority render first (background), higher priority renders last (foreground).

### Minimal 2D Scene Setup

Every 2D scene needs three things:

```cpp
#include "simulant/simulant.h"

using namespace smlt;

class My2DScene : public Scene {
public:
    My2DScene(Window* window) : Scene(window) {}

    void on_load() override {
        // 1. Create a 2D camera with orthographic projection
        auto camera = create_child<Camera2D>();
        camera->set_orthographic_projection(
            0, window->width(),       // left, right (pixel coordinates)
            0, window->height(),      // bottom, top
            -1.0f, 1.0f              // near, far
        );

        // 2. Create a sprite
        auto sprite = create_child<Sprite>();

        // 3. Create a render pipeline (Layer)
        auto layer = compositor->create_layer(this, camera);
        layer->viewport->set_color(Color(0.1f, 0.1f, 0.2f, 1.0f)); // Background color
    }
};
```

---

## 2. The Sprite Node

The `Sprite` class is a `StageNode` subclass designed for 2D rendering. It inherits from `ContainerNode` and `KeyFrameAnimated`, meaning it can have children and supports frame-based animation out of the box.

### Internal Structure

A Sprite internally manages:

- A **quad mesh** (4 vertices forming a rectangle)
- A **material** with alpha blending
- An **Actor** child that holds the mesh for rendering
- A **KeyFrameAnimationState** for frame animation

### Creating a Sprite

```cpp
// Method 1: As a child of the scene
auto sprite = create_child<Sprite>();

// Method 2: With an immediate name
auto player = create_child<Sprite>()->set_name_and_get("Player");

// Method 3: As a child of another node (for grouping)
auto player_group = create_child<Stage>()->set_name_and_get("PlayerGroup");
auto player_sprite = player_group->create_child<Sprite>();
```

### Setting Render Dimensions

Sprites must know how large to appear in world space:

```cpp
// Set both width and height (in world units)
sprite->set_render_dimensions(2.0f, 2.0f);

// Set from width only -- height is calculated from sprite sheet aspect ratio
sprite->set_render_dimensions_from_width(2.0f);

// Set from height only -- width is calculated from sprite sheet aspect ratio
sprite->set_render_dimensions_from_height(2.0f);
```

> **Important:** You must call `set_spritesheet()` before using `set_render_dimensions_from_width()` or `set_render_dimensions_from_height()`, because these methods need the sprite sheet's frame aspect ratio.

### Setting Render Priority

Control the draw order of sprites:

```cpp
// Render behind most things
sprite->set_render_priority(RENDER_PRIORITY_BACKGROUND);

// Main game layer (default)
sprite->set_render_priority(RENDER_PRIORITY_MAIN);

// Render on top of everything
sprite->set_render_priority(RENDER_PRIORITY_FOREGROUND);
```

### Transparency

```cpp
sprite->set_alpha(0.5f);  // 50% transparent

// Read current alpha
float a = sprite->alpha();  // Returns 0.5f
```

### Flipping

Flip a sprite horizontally or vertically (useful for facing direction):

```cpp
sprite->flip_horizontally(true);   // Mirror horizontally
sprite->flip_vertically(false);    // Normal vertically
```

### Accessing the Internal Actor

The sprite's internal rendering Actor is exposed via a property:

```cpp
ActorPtr actor = sprite->actor;  // Access the underlying Actor
```

---

## 3. Sprite Sheets and Configuration

A **sprite sheet** is a single texture image containing multiple frames laid out in a grid. The Sprite node reads frames from this texture by calculating the correct UV (texture coordinate) region each frame.

### Sprite Sheet Layout

Frames are arranged left-to-right, then top-to-bottom:

```
+------+------+------+------+
|  0   |  1   |  2   |  3   |   <- Row 0 (frames 0-3)
+------+------+------+------+
|  4   |  5   |  6   |  7   |   <- Row 1 (frames 4-7)
+------+------+------+------+
|  8   |  9   |  10  |  11  |   <- Row 2 (frames 8-11)
+------+------+------+------+
```

Frame N is located at:
- `column = N % frames_per_row`
- `row = N / frames_per_row`

The number of frames per row is calculated as `image_width / frame_width`.

### SpritesheetAttrs

The `SpritesheetAttrs` structure configures how frames are laid out:

```cpp
struct SpritesheetAttrs {
    uint32_t margin = 0;              // Margin around the entire sheet edge
    uint32_t spacing = 0;             // Gap (in pixels) between adjacent frames
    uint32_t padding_vertical = 0;    // Extra padding inside frames (vertical)
    uint32_t padding_horizontal = 0;  // Extra padding inside frames (horizontal)
};
```

Visual representation:

```
<--margin-->  <--spacing-->  <--spacing-->  <--margin-->
+-----------+--------------+--------------+-----------+
|           |              |              |           |
|  Frame 0  |   Frame 1    |   Frame 2    |           |
|           |              |              |           |
+-----------+--------------+--------------+-----------+
```

### Setting Up a Sprite Sheet

```cpp
// Load the sprite sheet texture
auto texture = assets->load_texture("textures/player_sheet.png");

// Configure the sheet
SpritesheetAttrs attrs;
attrs.margin = 0;
attrs.spacing = 2;                  // 2-pixel gap between frames
attrs.padding_horizontal = 0;
attrs.padding_vertical = 0;

// Apply the sprite sheet: frame is 64x64 pixels
sprite->set_spritesheet(texture, 64, 64, attrs);

// Set the world-space render size
sprite->set_render_dimensions(1.5f, 1.5f);
```

> **Note:** When you call `set_spritesheet()`, the material is automatically created with alpha blending (`BLEND_ALPHA`) enabled.

---

## 4. Creating Sprites from Textures

### From a Single Texture (Non-Animated)

If your sprite is a single image (not a sheet), treat the entire image as one frame:

```cpp
auto texture = assets->load_texture("textures/icon.png");

SpritesheetAttrs attrs;
attrs.margin = 0;
attrs.spacing = 0;

auto icon = create_child<Sprite>();
icon->set_spritesheet(texture, texture->width(), texture->height(), attrs);

// Render at 100x100 world units
icon->set_render_dimensions(1.0f, 1.0f);
```

### From an External Tool's Sprite Sheet

Many tools (TexturePacker, Aseprite, etc.) produce sprite sheets with specific configurations:

```cpp
// TexturePacker-style sheet with 2px spacing
auto texture = assets->load_texture("textures/characters.png");

SpritesheetAttrs attrs;
attrs.margin = 2;
attrs.spacing = 2;

auto character = create_child<Sprite>();
character->set_spritesheet(texture, 32, 48, attrs);
character->set_render_dimensions(1.0f, 1.5f);
```

### Changing a Sprite's Texture at Runtime

To swap a sprite's texture (e.g., change appearance), call `set_spritesheet()` again:

```cpp
auto new_texture = assets->load_texture("textures/player_hurt.png");
sprite->set_spritesheet(new_texture, 64, 64);
```

### Positioning a Sprite

Since `Sprite` is a `StageNode`, it has a `transform` property:

```cpp
// Position in 2D (Z stays at 0 for pure 2D)
sprite->transform->set_position(Vec3(5.0f, 3.0f, 0.0f));

// Move relative to current position
sprite->transform->translate(Vec3(1.0f, 0.0f, 0.0f));

// Scale (note: this scales the world-space size, not the texture)
sprite->transform->set_scale(Vec3(2.0f, 2.0f, 1.0f));

// Rotation (rarely needed for sprites, but supported)
sprite->transform->rotate(Vec3(0, 0, 1), Degrees(45));
```

---

## 5. Sprite Animations (Frame-by-Frame)

The `Sprite` class inherits from `KeyFrameAnimated`, which provides a simple but powerful frame-based animation system. You define named animations by specifying frame ranges and an FPS.

### Defining Animations

```cpp
// Load and configure the sprite sheet
auto texture = assets->load_texture("textures/player_run.png");
sprite->set_spritesheet(texture, 64, 64);
sprite->set_render_dimensions(1.0f, 1.0f);

// Define animations by name, start frame, end frame, and FPS
sprite->add_animation("idle",  0,  3, 8.0f);   // Frames 0-3 at 8 FPS
sprite->add_animation("run",   4, 11, 12.0f);  // Frames 4-11 at 12 FPS
sprite->add_animation("jump",  12, 14, 10.0f); // Frames 12-14 at 10 FPS

// Or use the default FPS for all animations
sprite->set_default_fps(10.0f);
sprite->add_animation("attack", 15, 19);  // Uses default FPS (10.0)
```

### Frame Calculation

The engine determines which texture region to display based on the current frame number. Frames are numbered sequentially across the entire sprite sheet, row by row:

```cpp
// For a 256x128 texture with 64x64 frames:
// frames_per_row = 256 / 64 = 4
//
// Frame 0  -> row 0, col 0
// Frame 3  -> row 0, col 3
// Frame 4  -> row 1, col 0
// Frame 7  -> row 1, col 3
```

---

## 6. Animation States and Playback

Each `Sprite` has an `animations` property that holds its `KeyFrameAnimationState`. This is the playback controller for sprite animations.

### Playing Animations

```cpp
// Play a specific animation
sprite->animations->play_animation("run");

// Play the first animation that was defined
sprite->animations->play_first_animation();

// Check the current frame
uint32_t frame = sprite->animations->current_frame();
```

### Queueing Animations

Queue an animation to play automatically after the current one finishes:

```cpp
sprite->animations->play_animation("jump");
sprite->animations->queue_next_animation("idle");
// After "jump" finishes, "idle" starts playing
```

### Updating Animations

Animation updates happen automatically through `Sprite::on_update(dt)`. You do not need to call anything manually -- the engine advances the animation every frame.

### Responding to Frame Changes

Internally, the sprite's `refresh_animation_state` callback is invoked each frame, which recalculates the UV coordinates. You can also access the animation state to check current progress:

```cpp
void MyScene::on_update(float dt) {
    Scene::on_update(dt);

    auto state = sprite->animations.get();
    if (state) {
        uint32_t current = state->current_frame();
        uint32_t next = state->next_frame();
        float interp = state->interp();  // 0.0 - 1.0 blend factor

        // You can trigger events on specific frames:
        if (current == 5) {
            // Play footstep sound on frame 5 of the run animation
        }
    }
}
```

### Changing Default FPS

```cpp
sprite->set_default_fps(15.0f);  // All future add_animation calls use 15 FPS
```

---

## 7. Camera2D for 2D Games

`Camera2D` is a specialized camera node designed for 2D rendering. It simplifies orthographic camera setup by providing intuitive parameters.

### Creating a Camera2D

```cpp
auto camera = create_child<Camera2D>();
```

### Camera2D Parameters

`Camera2D` exposes these parameters (usable via the Params system):

| Parameter | Default | Description |
|-----------|---------|-------------|
| `xmag` | 1.0f | Width of the view |
| `ymag` | 1.0f | Height of the view |
| `znear` | 1.0f | Near clipping distance |
| `zfar` | 100.0f | Far clipping distance |

### Positioning the Camera

Move the camera to pan around your 2D world:

```cpp
// Center the camera at world position (100, 50)
camera->transform->set_position(Vec3(100.0f, 50.0f, 10.0f));

// Pan the camera during gameplay
void on_update(float dt) override {
    camera->transform->translate(Vec3(player_velocity.x * dt, player_velocity.y * dt, 0));
}
```

### Using Camera2D in a Pipeline

```cpp
// Create the render layer
auto layer = compositor->create_layer(this, camera);
layer->viewport->set_color(Color(0.2f, 0.3f, 0.5f, 1.0f));

// Optional: set a specific render priority for this layer
// layer->set_priority(RENDER_PRIORITY_MAIN);
```

---

## 8. Orthographic Projection

Orthographic projection renders objects at a constant size regardless of distance. This is essential for 2D games where perspective distortion would break the visual consistency.

### Setting Orthographic Projection

The `Camera` base class provides two methods:

```cpp
// Method 1: Explicit bounds
camera->set_orthographic_projection(
    left, right,    // Horizontal bounds
    bottom, top,    // Vertical bounds
    near, far       // Depth bounds (usually -1.0 to 1.0 for 2D)
);

// Method 2: From desired height (auto-calculates width from aspect ratio)
camera->set_orthographic_projection_from_height(
    desired_height_in_world_units,
    aspect_ratio
);
```

### Pixel-Perfect Camera

For pixel-perfect rendering where 1 world unit = 1 screen pixel:

```cpp
camera->set_orthographic_projection(
    0, window->width(),       // 0 to screen width
    0, window->height(),      // 0 to screen height
    -1.0f, 1.0f              // Near/far
);
```

This maps the camera view to exactly match the screen's pixel dimensions. A sprite with `set_render_dimensions(64, 64)` will appear as 64x64 pixels on screen.

### World-Unit Camera

For games where you think in game units rather than pixels:

```cpp
// 20 units wide, maintaining aspect ratio
float world_width = 20.0f;
float aspect = float(window->width()) / float(window->height());
float world_height = world_width / aspect;

camera->set_orthographic_projection(
    -world_width / 2, world_width / 2,   // Centered at origin
    -world_height / 2, world_height / 2,
    -1.0f, 1.0f
);
```

With this setup, a sprite with `set_render_dimensions(1.0f, 1.0f)` takes up 1/20th of the screen width.

### Handling Window Resizing

When the window resizes, update the orthographic projection:

```cpp
void on_window_resized() override {
    camera->set_orthographic_projection(
        0, window->width(),
        0, window->height(),
        -1.0f, 1.0f
    );
}
```

---

## 9. 2D Input Handling

Simulant's input system uses **InputAxis** abstractions. An axis maps one or more physical inputs (keyboard keys, mouse buttons, joystick axes) to a single logical value between -1.0 and 1.0.

### Defining Input Axes

Define axes once, typically during scene setup:

```cpp
void on_load() override {
    // Horizontal movement: A/D or Left/Right arrow keys
    auto axis_left = input->new_axis("move_left");
    axis_left->set_positive_keyboard_key(KEYBOARD_CODE_A);
    axis_left->set_negative_keyboard_key(KEYBOARD_CODE_LEFT);

    auto axis_right = input->new_axis("move_right");
    axis_right->set_positive_keyboard_key(KEYBOARD_CODE_D);
    axis_right->set_negative_keyboard_key(KEYBOARD_CODE_RIGHT);

    // Jump: Space or W or Up arrow
    auto axis_jump = input->new_axis("jump");
    axis_jump->set_positive_keyboard_key(KEYBOARD_CODE_SPACE);

    // Alternatively, use a simpler approach with opposite keys on one axis:
    auto h_axis = input->new_axis("horizontal");
    h_axis->set_positive_keyboard_key(KEYBOARD_CODE_D);
    h_axis->set_negative_keyboard_key(KEYBOARD_CODE_A);

    auto v_axis = input->new_axis("vertical");
    v_axis->set_positive_keyboard_key(KEYBOARD_CODE_W);
    v_axis->set_negative_keyboard_key(KEYBOARD_CODE_S);
}
```

### Reading Input Values

```cpp
void on_update(float dt) override {
    // Get a continuous value between -1.0 and 1.0
    float h = input->axis_value("horizontal");
    float v = input->axis_value("vertical");

    // Move the sprite
    sprite->transform->translate(Vec3(h * speed * dt, v * speed * dt, 0));
}
```

### Detecting Press and Release

For single-frame events (like jumping):

```cpp
void on_update(float dt) override {
    // True only on the frame the key was first pressed
    if (input->axis_was_pressed("jump")) {
        jump();
    }

    // True only on the frame the key was released
    if (input->axis_was_released("jump")) {
        // Shorten jump if player releases jump button early
    }
}
```

### Hard (Digital) Axis Values

For boolean-style inputs (fully on or off):

```cpp
int8_t jump_pressed = input->axis_value_hard("jump");
if (jump_pressed == 1) {
    // Jump button is held down
}
```

### Dead Zones for Joysticks

When using gamepad joysticks, set a dead zone to prevent drift:

```cpp
auto joy_axis = input->new_axis("joystick_x");
joy_axis->set_joystick_axis(JOYSTICK_AXIS_0);
joy_axis->set_dead_zone(0.2f);  // Ignore small movements

// When reading:
float value = joy_axis->value(DEAD_ZONE_BEHAVIOUR_RADIAL);  // Default
```

### Complete 2D Input Example

```cpp
class PlayerController : public StageNode {
public:
    FindResult<Sprite> sprite = FindDescendent("PlayerSprite", this);

    float move_speed = 5.0f;
    float jump_force = 10.0f;
    bool is_grounded = true;

    void on_update(float dt) override {
        float h = input->axis_value("horizontal");

        // Horizontal movement
        sprite->transform->translate(Vec3(h * move_speed * dt, 0, 0));

        // Flip sprite based on direction
        if (h > 0.1f) {
            sprite->flip_horizontally(false);
        } else if (h < -0.1f) {
            sprite->flip_horizontally(true);
        }

        // Jump
        if (input->axis_was_pressed("jump") && is_grounded) {
            is_grounded = false;
            velocity_y = jump_force;
        }
    }
};
```

---

## 10. Tile-Based Rendering

Simulant does not include a built-in tilemap system, but tile-based rendering can be implemented efficiently using Sprites.

### Single-Sprite Tiles

The simplest approach: create a `Sprite` for each visible tile.

```cpp
// Tile dimensions in pixels
const uint32_t TILE_WIDTH = 32;
const uint32_t TILE_HEIGHT = 32;

// Tile map data (0 = empty, 1 = grass, 2 = wall, etc.)
std::vector<std::vector<int>> tile_map = {
    {0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 2, 2, 2, 0, 0, 0},
    {0, 1, 1, 1, 1, 1, 0, 0},
    {1, 1, 1, 1, 1, 1, 1, 0},
};

// Create sprites for each tile
auto tile_texture = assets->load_texture("textures/tiles.png");

for (size_t y = 0; y < tile_map.size(); ++y) {
    for (size_t x = 0; x < tile_map[y].size(); ++x) {
        int tile_type = tile_map[y][x];
        if (tile_type == 0) continue;  // Skip empty tiles

        auto tile = create_child<Sprite>();
        tile->set_name_and_get(
            "Tile_" + std::to_string(x) + "_" + std::to_string(y)
        );

        // Each tile type corresponds to a frame in the sprite sheet
        SpritesheetAttrs attrs;
        tile->set_spritesheet(tile_texture, TILE_WIDTH, TILE_HEIGHT, attrs);
        tile->set_render_dimensions(1.0f, 1.0f);

        // Position in world space
        tile->transform->set_position(Vec3(
            float(x), float(y), 0.0f
        ));
    }
}
```

### Reusing Materials for Performance

For large tilemaps, creating a Sprite per tile creates many Actors. A more efficient approach uses a single mesh with multiple submeshes or a shared material:

```cpp
// All tiles share the same texture and material
auto tile_texture = assets->load_texture("textures/tiles.png");
auto tile_material = assets->create_material_from_texture(tile_texture);
tile_material->set_blend_func(BLEND_ALPHA);
```

### Camera Culling for Large Tilemaps

For very large maps, only create sprites for tiles near the camera:

```cpp
void update_visible_tiles(const Vec3& camera_pos, int view_radius) {
    // Deactivate tiles outside view
    // Activate/create tiles within view_radius of camera_pos
    // This is a simple form of frustum culling for tilemaps
}
```

---

## 11. 2D Physics

Simulant's physics system (based on the Bounce library) is inherently 3D, but it works well for 2D games by constraining movement to a plane.

### Setting Up 2D Physics

The key to 2D physics is restricting movement to the XY plane (Z is locked):

```cpp
class Physics2DScene : public Scene {
public:
    void on_load() override {
        // Set gravity pointing "down" in 2D (negative Y)
        physics->set_gravity(Vec3(0, -9.81f, 0));

        create_ground();
        create_player();
    }

    void on_fixed_update(float step) override {
        // Physics steps run at fixed intervals
        // Sync visual positions with physics bodies
        sync_physics_to_visuals();
    }
};
```

### Creating a 2D Ground Body

```cpp
void create_ground() {
    // Visual ground
    auto ground_sprite = create_child<Sprite>();
    auto ground_tex = assets->load_texture("textures/ground.png");
    ground_sprite->set_spritesheet(ground_tex, 64, 64);
    ground_sprite->set_render_dimensions(20.0f, 1.0f);
    ground_sprite->transform->set_position(Vec3(10.0f, -0.5f, 0));
    ground_sprite->set_name_and_get("Ground");

    // Physics body (static = immovable)
    auto ground_body = ground_sprite->create_child<StaticBody>();
    ground_body->create_box_fixture(Vec3(20.0f, 1.0f, 1.0f));
    ground_body->set_restitution(0.0f);  // No bounce
    ground_body->set_friction(0.8f);
}
```

### Creating a 2D Player with Physics

```cpp
void create_player() {
    // Visual
    player_sprite = create_child<Sprite>();
    auto tex = assets->load_texture("textures/player.png");
    player_sprite->set_spritesheet(tex, 32, 48);
    player_sprite->set_render_dimensions(0.8f, 1.2f);
    player_sprite->transform->set_position(Vec3(5.0f, 5.0f, 0));
    player_sprite->set_name_and_get("Player");

    // Physics body (dynamic = affected by gravity)
    player_body = player_sprite->create_child<DynamicBody>();
    player_body->create_box_fixture(Vec3(0.8f, 1.2f, 0.5f));
    player_body->set_mass(1.0f);
    player_body->set_restitution(0.0f);
    player_body->set_linear_damping(0.0f);
    player_body->set_angular_damping(1.0f);  // Prevent rotation in 2D
}
```

### Moving a 2D Character with Physics

```cpp
void on_update(float dt) override {
    float h = input->axis_value("horizontal");

    if (player_body) {
        // Apply horizontal force for movement
        Vec3 force(h * 50.0f, 0, 0);
        player_body->apply_force(force);

        // Jump
        if (input->axis_was_pressed("jump") && is_grounded_) {
            player_body->apply_impulse(Vec3(0, 8.0f, 0));
            is_grounded_ = false;
        }
    }
}

void sync_physics_to_visuals() {
    if (player_body && player_sprite) {
        Vec3 phys_pos = player_body->position();
        player_sprite->transform->set_position(Vec3(
            phys_pos.x, phys_pos.y, 0.0f  // Keep Z at 0 for 2D
        ));
    }
}
```

### Collision Detection in 2D

Use collision listeners to react to contacts:

```cpp
class PlayerCollisionListener : public CollisionListener {
public:
    void on_collision_enter(const Collision& collision) override {
        if (collision.other_body->type() == PHYSICS_BODY_TYPE_STATIC) {
            // Hit the ground
            player_ref->is_grounded_ = true;
        }
    }

    void on_collision_exit(const Collision& collision) override {
        if (collision.other_body->type() == PHYSICS_BODY_TYPE_STATIC) {
            // Left the ground
            player_ref->is_grounded_ = false;
        }
    }

private:
    GameScene* player_ref = nullptr;
};

// Register the listener
auto listener = new PlayerCollisionListener();
listener->player_ref = this;
player_body->register_collision_listener(listener);
```

### Kinematic Bodies for 2D Moving Platforms

```cpp
// Create a moving platform
auto platform = create_child<Sprite>();
platform->set_spritesheet(tex, 64, 32);
platform->set_render_dimensions(3.0f, 1.0f);
platform->set_name_and_get("Platform");

auto platform_body = platform->create_child<KinematicBody>();
platform_body->create_box_fixture(Vec3(3.0f, 1.0f, 0.5f));

// Move it in on_update
void on_update(float dt) override {
    platform_x += direction * 2.0f * dt;
    if (platform_x > 15.0f || platform_x < 2.0f) {
        direction *= -1;
    }
    platform_body->set_position(Vec3(platform_x, 3.0f, 0));
}
```

---

## 12. Complete 2D Game Example -- Simple Platformer

This section builds a complete, minimal 2D platformer demonstrating all the concepts covered above.

### Project Structure

```
my_platformer/
  src/
    main.cpp
  assets/
    textures/
      player_sheet.png   (64x64 frames)
      tile_sheet.png     (32x32 frames)
      background.png
```

### Full Source Code

```cpp
#include "simulant/simulant.h"

using namespace smlt;

// ============================================================
// Platformer Game Scene
// ============================================================
class PlatformerScene : public Scene {
public:
    PlatformerScene(Window* window) : Scene(window) {}

    void on_load() override {
        // ---- 2D Camera (pixel-aligned) ----
        camera_ = create_child<Camera2D>();
        camera_->set_orthographic_projection(
            0, window->width(),
            0, window->height(),
            -1.0f, 1.0f
        );

        // ---- Render pipeline ----
        auto layer = compositor->create_layer(this, camera_);
        layer->viewport->set_color(Color(0.15f, 0.15f, 0.25f, 1.0f));

        // ---- Input axes ----
        setup_input();

        // ---- Load textures ----
        auto player_tex = assets->load_texture("textures/player_sheet.png");
        auto tile_tex = assets->load_texture("textures/tile_sheet.png");

        // ---- Physics ----
        physics->set_gravity(Vec3(0, -9.81f, 0));

        // ---- Build level ----
        build_level(tile_tex);

        // ---- Create player ----
        create_player(player_tex);

        // ---- Collision listener ----
        auto* listener = new GroundListener(this);
        player_body_->register_collision_listener(listener);
    }

    void on_update(float dt) override {
        Scene::on_update(dt);
        handle_input(dt);
        update_camera();
    }

    void on_fixed_update(float step) override {
        // Sync physics body to visual
        if (player_body_ && player_sprite_) {
            Vec3 pos = player_body_->position();
            player_sprite_->transform->set_position(
                Vec3(pos.x, pos.y, 0.0f)
            );
        }
    }

private:
    // ---- Input setup ----
    void setup_input() {
        auto h = input->new_axis("horizontal");
        h->set_positive_keyboard_key(KEYBOARD_CODE_D);
        h->set_negative_keyboard_key(KEYBOARD_CODE_LEFT);
        // Also support arrow keys on the same axis
        auto h2 = input->new_axis("horizontal");
        h2->set_positive_keyboard_key(KEYBOARD_CODE_RIGHT);
        h2->set_negative_keyboard_key(KEYBOARD_CODE_A);

        auto jump = input->new_axis("jump");
        jump->set_positive_keyboard_key(KEYBOARD_CODE_SPACE);

        auto quit = input->new_axis("quit");
        quit->set_positive_keyboard_key(KEYBOARD_CODE_ESCAPE);
    }

    // ---- Build the level with tiles ----
    void build_level(TexturePtr tile_tex) {
        const int TILE_SIZE = 32;

        // 0=empty, 1=ground, 2=wall, 3=platform
        std::vector<std::string> level_data = {
            "............................",
            "............................",
            "............................",
            "............................",
            ".........333................",
            "............................",
            "..................333.......",
            "............................",
            "......22.........22.........",
            "1111111111111111111111111111",
        };

        SpritesheetAttrs attrs;
        attrs.spacing = 0;

        for (size_t row = 0; row < level_data.size(); ++row) {
            for (size_t col = 0; col < level_data[row].size(); ++col) {
                char tile = level_data[row][col];
                if (tile == '.') continue;

                auto tile_sprite = create_child<Sprite>();
                tile_sprite->set_spritesheet(tile_tex, TILE_SIZE, TILE_SIZE, attrs);
                tile_sprite->set_render_dimensions(1.0f, 1.0f);
                tile_sprite->transform->set_position(
                    Vec3(float(col), float(level_data.size() - row), 0.0f)
                );

                // Add physics for solid tiles
                if (tile != '.') {
                    auto body = tile_sprite->create_child<StaticBody>();
                    body->create_box_fixture(Vec3(1.0f, 1.0f, 0.5f));
                    body->set_friction(0.6f);
                }
            }
        }
    }

    // ---- Create the player character ----
    void create_player(TexturePtr tex) {
        // Visual
        player_sprite_ = create_child<Sprite>();

        SpritesheetAttrs attrs;
        attrs.spacing = 2;
        player_sprite_->set_spritesheet(tex, 64, 64, attrs);
        player_sprite_->set_render_dimensions(1.0f, 1.0f);
        player_sprite_->transform->set_position(Vec3(3.0f, 5.0f, 0.0f));
        player_sprite_->set_name_and_get("Player");

        // Animations (assuming a sprite sheet layout)
        player_sprite_->add_animation("idle", 0, 3, 8.0f);
        player_sprite_->add_animation("run",  4, 11, 12.0f);
        player_sprite_->add_animation("jump", 12, 13, 6.0f);

        // Start with idle
        player_sprite_->animations->play_animation("idle");

        // Physics body
        player_body_ = player_sprite_->create_child<DynamicBody>();
        player_body_->create_box_fixture(Vec3(0.6f, 0.9f, 0.5f));
        player_body_->set_mass(1.0f);
        player_body_->set_restitution(0.0f);
        player_body_->set_angular_damping(1.0f);  // Prevent tipping over
    }

    // ---- Input handling ----
    void handle_input(float dt) {
        if (!player_body_) return;

        // Quit
        if (input->axis_value_hard("quit") == 1) {
            app->stop_running();
            return;
        }

        float h = input->axis_value("horizontal");
        const float MOVE_SPEED = 8.0f;

        // Apply movement force
        player_body_->apply_force(Vec3(h * MOVE_SPEED, 0, 0));

        // Flip sprite based on movement direction
        if (h > 0.1f) {
            player_sprite_->flip_horizontally(false);
        } else if (h < -0.1f) {
            player_sprite_->flip_horizontally(true);
        }

        // Update animation state
        update_player_animation(h);

        // Jump
        if (input->axis_was_pressed("jump") && is_grounded_) {
            player_body_->apply_impulse(Vec3(0, 10.0f, 0));
            is_grounded_ = false;
        }
    }

    // ---- Animation state machine ----
    void update_player_animation(float h) {
        if (!player_sprite_ || !player_sprite_->animations) return;

        if (!is_grounded_) {
            player_sprite_->animations->play_animation("jump");
        } else if (std::abs(h) > 0.1f) {
            player_sprite_->animations->play_animation("run");
        } else {
            player_sprite_->animations->play_animation("idle");
        }
    }

    // ---- Follow camera ----
    void update_camera() {
        if (!player_sprite_) return;

        Vec3 player_pos = player_sprite_->transform->position();

        // Camera follows player with smoothing
        Vec3 target(player_pos.x, player_pos.y, 10.0f);
        Vec3 current = camera_->transform->position();
        Vec3 smoothed = current + (target - current) * 5.0f * (1.0f / 60.0f);
        camera_->transform->set_position(smoothed);
    }

    // ---- Ground collision listener ----
    class GroundListener : public CollisionListener {
    public:
        GroundListener(PlatformerScene* scene) : scene_(scene) {}

        void on_collision_enter(const Collision& collision) override {
            if (collision.other_body->type() == PHYSICS_BODY_TYPE_STATIC) {
                scene_->is_grounded_ = true;
            }
        }

        void on_collision_exit(const Collision& collision) override {
            if (collision.other_body->type() == PHYSICS_BODY_TYPE_STATIC) {
                scene_->is_grounded_ = false;
            }
        }

    private:
        PlatformerScene* scene_;
    };

    // ---- Member variables ----
    CameraPtr camera_;
    Sprite* player_sprite_ = nullptr;
    DynamicBody* player_body_ = nullptr;
    bool is_grounded_ = true;
};

// ============================================================
// Application
// ============================================================
class PlatformerApp : public Application {
public:
    PlatformerApp(const AppConfig& config) : Application(config) {}

private:
    bool init() override {
        scenes->register_scene<PlatformerScene>("game");
        scenes->activate("game");
        return true;
    }
};

int main(int argc, char* argv[]) {
    _S_UNUSED(argc);
    _S_UNUSED(argv);

    AppConfig config;
    config.title = "2D Platformer";
    config.fullscreen = false;
    config.width = 800;
    config.height = 600;
    config.log_level = LOG_LEVEL_INFO;

    PlatformerApp app(config);
    return app.run();
}
```

---

## 13. Best Practices for 2D Games

### 1. Use Pixel-Aligned Cameras for Pixel Art

```cpp
camera->set_orthographic_projection(
    0, window->width(), 0, window->height(), -1.0f, 1.0f
);
```

This ensures 1 world unit = 1 screen pixel, giving crisp pixel art rendering.

### 2. Organize Sprites in a Scene Graph Hierarchy

Group related sprites under parent `Stage` nodes:

```cpp
auto background_layer = create_child<Stage>()->set_name_and_get("Background");
auto game_layer = create_child<Stage>()->set_name_and_get("Game");
auto ui_layer = create_child<Stage>()->set_name_and_get("UI");

// Add sprites to each layer
auto bg = background_layer->create_child<Sprite>();
auto player = game_layer->create_child<Sprite>();
auto health_bar = ui_layer->create_child<Sprite>();
```

### 3. Use Render Priority for Layering

```cpp
// Background elements
bg_sprite->set_render_priority(RENDER_PRIORITY_BACKGROUND);

// Game elements (default)
player->set_render_priority(RENDER_PRIORITY_MAIN);

// UI elements (on top)
hud->set_render_priority(RENDER_PRIORITY_FOREGROUND);
```

### 4. Flip Sprites Instead of Duplicating Textures

Save texture memory by reusing the same sheet and flipping:

```cpp
// Walking right
player->flip_horizontally(false);

// Walking left
player->flip_horizontally(true);
```

### 5. Keep Sprite Sheets Organized

- Group related animations in the same sheet (idle, run, jump for one character).
- Use consistent frame sizes within a sheet.
- Leave at least 1-2 pixels of spacing between frames to avoid bleeding.
- Use power-of-two texture sizes (256, 512, 1024) for best GPU compatibility.

### 6. Implement an Animation State Machine

Don't scatter animation calls throughout your code. Centralize them:

```cpp
enum PlayerState { IDLE, RUNNING, JUMPING, FALLING, ATTACKING };

void update_animation(PlayerState state) {
    if (state == current_state_) return;  // No change needed

    switch (state) {
        case IDLE:
            sprite->animations->play_animation("idle");
            break;
        case RUNNING:
            sprite->animations->play_animation("run");
            break;
        case JUMPING:
            sprite->animations->play_animation("jump");
            break;
    }
    current_state_ = state;
}
```

### 7. Use `set_render_dimensions_from_width/Height`

When you want sprites to render at their native pixel size:

```cpp
// If frame is 64x64 pixels and you want 64 world units
sprite->set_render_dimensions(64.0f, 64.0f);

// Or use aspect-ratio-aware sizing:
sprite->set_render_dimensions_from_width(64.0f);
```

### 8. Disable Angular Damping for 2D Physics Objects

Physics bodies naturally rotate in 3D. For 2D games, prevent this:

```cpp
body->set_angular_damping(1.0f);  // Maximum damping = no rotation
```

### 9. Constrain Camera Movement to 2D

Keep the camera's Z position constant:

```cpp
camera->transform->set_position(Vec3(x, y, 10.0f));  // Z stays at 10
```

### 10. Use Separate Cameras for UI and Game World

```cpp
// Game camera (follows player)
game_camera_ = create_child<Camera2D>();
game_camera_->set_orthographic_projection(
    -10, 10, -6, 6, -1.0f, 1.0f
);

// UI camera (pixel-aligned, fixed)
ui_camera_ = create_child<Camera2D>();
ui_camera_->set_orthographic_projection(
    0, window->width(), 0, window->height(), -1.0f, 1.0f
);

// Two separate render pipelines
auto game_layer = compositor->create_layer(game_stage_, game_camera_);
auto ui_layer = compositor->create_layer(ui_stage_, ui_camera_);
ui_layer->set_priority(RENDER_PRIORITY_FOREGROUND);
```

---

## 14. Performance Considerations

### Draw Calls and Batching

Each Sprite creates its own Actor with a mesh and material. This means **each sprite is a separate draw call**. For large numbers of sprites:

- **Tilemaps**: Consider building a single mesh with UV-mapped quads for all visible tiles instead of individual sprites.
- **Shared materials**: Sprites using the same texture share a material, which can help the renderer batch draw calls where possible.

### Sprite Sheet Size Limits

- Keep sprite sheets at reasonable sizes (2048x2048 or smaller).
- Very large sheets may not be supported on all platforms (especially embedded/constrained platforms like Dreamcast).
- Split animations across multiple sheets if needed.

### Animation Frame Rate

The `KeyFrameAnimationState` internally uses a `Throttle` capped at 60 FPS. Animations will not update more frequently than this, which is appropriate since sprite animations are typically 8-15 FPS.

### Memory Usage

Each Sprite allocates:
- One quad mesh (4 vertices)
- One material
- One Actor child
- One `KeyFrameAnimationState`

For hundreds of sprites, this overhead is minimal. For thousands, consider mesh-based approaches.

### Physics Performance

- Use simple collider shapes (boxes, spheres) instead of triangle meshes.
- The physics simulation runs at a fixed timestep. On slower hardware, reduce the simulation frequency.
- Disable physics bodies that are far off-screen.

### Camera and Culling

Simulant's built-in frustum culling works for sprites since they are Actors internally. However:

- For orthographic 2D cameras, culling is less effective because the view covers a large area.
- Implement manual visibility culling for very large 2D worlds:

```cpp
void update_visible_sprites(const Vec3& camera_pos, float view_width, float view_height) {
    for (auto& entry : all_sprites_) {
        Sprite* sprite = entry.second;
        Vec3 pos = sprite->transform->position();

        bool in_view = (
            pos.x >= camera_pos.x - view_width / 2 &&
            pos.x <= camera_pos.x + view_width / 2 &&
            pos.y >= camera_pos.y - view_height / 2 &&
            pos.y <= camera_pos.y + view_height / 2
        );

        sprite->set_visible(in_view);
    }
}
```

### Platform-Specific Notes

On constrained platforms (Dreamcast, PSP):

- Use smaller texture sizes (max 1024x1024 or less).
- Reduce the number of simultaneous sprites.
- Prefer simple animations (fewer frames) over complex ones.
- Avoid per-frame texture swaps; preload all needed textures.

---

## API Reference

### Sprite Key Methods

| Method | Description |
|--------|-------------|
| `set_spritesheet(tex, frame_w, frame_h, attrs)` | Configure the sprite sheet texture and frame size |
| `set_render_dimensions(w, h)` | Set world-space render size |
| `set_render_dimensions_from_width(w)` | Set size from width, height from aspect ratio |
| `set_render_dimensions_from_height(h)` | Set size from height, width from aspect ratio |
| `set_alpha(a)` | Set transparency (0.0 - 1.0) |
| `flip_horizontally(v)` / `flip_vertically(v)` | Mirror the sprite |
| `set_render_priority(p)` | Set draw order priority |
| `add_animation(name, start, end, fps)` | Define a named animation |
| `set_default_fps(fps)` | Set default FPS for animations |

### Camera2D Key Methods

| Method | Description |
|--------|-------------|
| `set_orthographic_projection(l, r, b, t, n, f)` | Set orthographic bounds explicitly |
| `set_orthographic_projection_from_height(h, ratio)` | Set from desired view height |

### KeyFrameAnimationState Key Methods

| Method | Description |
|--------|-------------|
| `play_animation(name)` | Play a named animation |
| `play_first_animation()` | Play the first defined animation |
| `queue_next_animation(name)` | Queue animation after current |
| `current_frame()` | Get the current frame index |
| `next_frame()` | Get the next frame index |
| `interp()` | Get interpolation factor (0.0 - 1.0) |
| `update(dt)` | Advance animation (called automatically) |

---

## See Also

- [Stage Nodes](../core-concepts/stage-nodes.md) -- Parent class and scene graph hierarchy
- [Animation Overview](../animation/overview.md) -- Skeleton and keyframe animation details
- [Cameras](../core-concepts/cameras.md) -- Camera system fundamentals
- [Input](../input/overview.md) -- Input axis configuration and handling
- [Physics](../physics/overview.md) -- Rigid body physics system
- [Render Pipelines](pipelines.md) -- Compositor and layer management
- [Textures](../textures.md) -- Texture loading and configuration
