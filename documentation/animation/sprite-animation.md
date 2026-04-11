# Sprite Animation

This document covers 2D sprite sheet animation in Simulant using the `Sprite` node and `KeyFrameAnimated` / `KeyFrameAnimationState` classes.

---

## Table of Contents

1. [Overview](#1-overview)
2. [Creating a Sprite](#2-creating-a-sprite)
3. [Sprite Sheet Configuration](#3-sprite-sheet-configuration)
4. [Defining Animations](#4-defining-animations)
5. [Playing Sprite Animations](#5-playing-sprite-animations)
6. [Animation State](#6-animation-state)
7. [How Sprite Animation Works](#7-how-sprite-animation-works)
8. [Flipping and Alpha](#8-flipping-and-alpha)
9. [Complete Example](#9-complete-example)

---

## 1. Overview

For 2D animation, Simulant provides the `Sprite` node. It works by updating texture coordinates on a quad mesh to display different frames from a sprite sheet texture.

Headers:
- `simulant/nodes/sprite.h` -- `Sprite`, `SpritesheetAttrs`
- `simulant/animation.h` -- `KeyFrameAnimated`, `KeyFrameAnimationState`, `AnimationUpdatedCallback`

The `Sprite` class inherits from both `KeyFrameAnimated` and `ContainerNode`:

```
Sprite
  |-- ContainerNode  (scene graph participation)
  |-- KeyFrameAnimated  (animation definition)
```

Each `Sprite` has a `KeyFrameAnimationState` accessible via the `animations` property, which handles playback:

```cpp
sprite->animations->play_animation("run");
sprite->animations->queue_next_animation("idle");
```

---

## 2. Creating a Sprite

Create a sprite node through the scene:

```cpp
auto sprite = scene->create_node<Sprite>();
```

Or as a child of an existing node:

```cpp
auto sprite = parent->create_child<Sprite>();
```

---

## 3. Sprite Sheet Configuration

### SpritesheetAttrs

The `SpritesheetAttrs` struct describes the layout of frames within the texture:

```cpp
struct SpritesheetAttrs {
    uint32_t margin = 0;             // Outer margin around the entire sheet
    uint32_t spacing = 0;            // Gap between adjacent frames
    uint32_t padding_vertical = 0;   // Extra vertical padding between frames
    uint32_t padding_horizontal = 0; // Extra horizontal padding between frames
};
```

### Setting the Sprite Sheet

```cpp
// Load the texture
auto texture = assets->load_texture("textures/character_sheet.png");

// Configure frame size and layout
SpritesheetAttrs attrs;
attrs.margin = 0;       // No outer margin
attrs.spacing = 2;      // 2 pixels between frames

// Set the sprite sheet: texture, frame width, frame height, optional attrs
sprite->set_spritesheet(texture, 64, 64, attrs);
```

### Render Dimensions

The sprite renders as a quad in world space. Set its size in world units:

```cpp
// Set both width and height
sprite->set_render_dimensions(1.0f, 1.0f);

// Set only width (height is calculated to preserve aspect ratio)
sprite->set_render_dimensions_from_width(1.0f);

// Set only height (width is calculated to preserve aspect ratio)
sprite->set_render_dimensions_from_height(1.0f);
```

> **Note:** Render dimensions are in **world units**, not pixels. A value of `1.0f` means the sprite occupies 1 world unit. The pixel size of frames in the sprite sheet only determines UV calculation, not render size.

---

## 4. Defining Animations

The `Sprite` inherits `add_animation()` from `KeyFrameAnimated`. Animations are defined by specifying a frame range and an optional FPS:

```cpp
// Add animation "run" using frames 0-7 at 10 frames per second
sprite->add_animation("run", 0, 7, 10.0f);

// Add animation "idle" using frames 8-15, using the default FPS
sprite->add_animation("idle", 8, 15);

// Set the default FPS (used when add_animation is called without FPS)
sprite->set_default_fps(12.0f);
```

Method signatures:

```cpp
void add_animation(const std::string& name,
                   uint32_t start_frame,
                   uint32_t end_frame,
                   float fps);

void add_animation(const std::string& name,
                   uint32_t start_frame,
                   uint32_t end_frame);  // Uses default_fps
```

### Default FPS

```cpp
void set_default_fps(float fps);
float default_fps() const;
```

The default FPS is `7.0f` (chosen as a common frame rate for MD2 models). Override it before defining animations that do not specify an explicit FPS.

### Animation Sequences

For more complex multi-stage animations, use `add_sequence()`:

```cpp
std::vector<AnimationSequenceStage> stages = {
    {"walk", 2.0f},   // Play "walk" for 2 seconds
    {"idle", 1.0f},   // Then play "idle" for 1 second
    {"run", 3.0f},    // Then play "run" for 3 seconds
};

sprite->add_sequence("patrol", stages);
```

---

## 5. Playing Sprite Animations

Sprite animations are controlled through the `animations` property, which returns a `KeyFrameAnimationState`:

```cpp
// Play a specific animation
sprite->animations->play_animation("run");

// Queue an animation to play after the current one finishes
sprite->animations->queue_next_animation("idle");

// Play the first defined animation
sprite->animations->play_first_animation();

// Play a sequence
sprite->animations->play_sequence("patrol");
```

### Override Playing Animation Duration

You can override the duration of the currently playing animation:

```cpp
sprite->animations->override_playing_animation_duration(5.0f);
```

### Animation Updates Are Automatic

The `Sprite::on_update(float dt)` method calls `animation_state_->update(dt)` automatically. You do not need to manually advance the animation state.

---

## 6. Animation State

The `KeyFrameAnimationState` tracks the current playback state:

```cpp
class KeyFrameAnimationState {
    void play_animation(const std::string& name);
    void queue_next_animation(const std::string& name);
    void play_first_animation();
    void play_sequence(const std::string& name);
    void update(float dt);

    uint32_t current_frame() const;  // Current keyframe index
    uint32_t next_frame() const;     // Next keyframe index
    float interp() const;            // Interpolation factor (0.0 - 1.0)
};
```

### Reading Animation State

```cpp
uint32_t frame = sprite->animations->current_frame();
uint32_t next = sprite->animations->next_frame();
float interp = sprite->animations->interp();

S_DEBUG("Frame {} -> {} (interp: {:.2f})", frame, next, interp);
```

### Throttle

The animation state uses a `Throttle` capped at 60 FPS to prevent excessive updates:

```cpp
Throttle throttle_ = Throttle(60);
```

This means the animation state will not update more than 60 times per second, regardless of the engine's frame rate.

---

## 7. How Sprite Animation Works

### Frame Layout

Frames in a sprite sheet are laid out left-to-right, top-to-bottom:

```
+------+------+------+
|  0   |  1   |  2   |  Row 0
+------+------+------+
|  3   |  4   |  5   |  Row 1
+------+------+------+
|  6   |  7   |  8   |  Row 2
+------+------+------+

Frame N is located at:
  column = N % frames_per_row
  row    = N / frames_per_row
```

The number of frames per row is calculated from the texture width and frame width.

### UV Coordinate Calculation

When the animation frame changes, the sprite recalculates its texture coordinates (UVs) to display the correct region of the sprite sheet. The calculation accounts for:

- **Margin**: Offset from the texture edges
- **Spacing**: Gap between adjacent frames
- **Padding**: Extra space between frames (horizontal and vertical)
- **Flipping**: Horizontal and vertical flip flags

The `update_texture_coordinates()` method performs this calculation internally.

### Animation Update Callback

The `KeyFrameAnimationState` takes a callback that fires each frame:

```cpp
typedef std::function<void (int32_t, int32_t, float)> AnimationUpdatedCallback;

auto state = std::make_shared<KeyFrameAnimationState>(
    sprite,
    [this](int32_t current_frame, int32_t next_frame, float interp) {
        // Called when the animation state updates
        this->on_animation_frame_changed(current_frame, next_frame, interp);
    }
);
```

For `Sprite`, this callback is set up internally and calls `update_texture_coordinates()` to update the mesh UVs.

---

## 8. Flipping and Alpha

### Flipping

Flip the sprite horizontally or vertically:

```cpp
sprite->flip_horizontally(true);   // Mirror horizontally
sprite->flip_vertically(true);     // Mirror vertically
```

This is useful for reusing a single walk cycle for both left and right movement:

```cpp
// Flip based on movement direction
if (moving_left) {
    sprite->flip_horizontally(true);
} else if (moving_right) {
    sprite->flip_horizontally(false);
}
```

### Alpha Transparency

```cpp
sprite->set_alpha(0.5f);   // 50% transparent
sprite->set_alpha(1.0f);   // Fully opaque (default)
sprite->set_alpha(0.0f);   // Fully transparent

float current_alpha = sprite->alpha();
```

---

## 9. Complete Example

This example demonstrates setting up a 2D character with multiple animations and input-driven playback:

```cpp
#include "simulant/simulant.h"

using namespace smlt;

enum CharacterAnimState {
    IDLE,
    RUNNING,
    JUMPING
};

class SpriteScene : public Scene {
public:
    SpriteScene(Window* window) : Scene(window) {}

    void on_load() override {
        // Load the sprite sheet
        auto texture = assets->load_texture("textures/hero.png");
        if (!texture) {
            S_ERROR("Failed to load sprite sheet texture!");
            return;
        }

        // Create the sprite
        sprite_ = create_child<Sprite>();

        // Configure sprite sheet (64x64 pixel frames, 2px spacing)
        SpritesheetAttrs attrs;
        attrs.spacing = 2;

        sprite_->set_spritesheet(texture, 64, 64, attrs);
        sprite_->set_render_dimensions(1.0f, 1.0f);

        // Define animations
        // Assuming a sprite sheet with the following layout:
        // Frames 0-3: idle
        // Frames 4-11: run
        // Frames 12-15: jump
        sprite_->add_animation("idle", 0, 3, 8.0f);
        sprite_->add_animation("run", 4, 11, 12.0f);
        sprite_->add_animation("jump", 12, 15, 10.0f);

        // Start with idle
        set_anim_state(IDLE);

        // Camera (orthographic for 2D)
        camera_ = create_child<Camera2D>();
        camera_->transform->set_position(Vec3(0, 0, 10));

        compositor->create_layer(sprite_, camera_)
            ->set_clear_flags(BUFFER_CLEAR_ALL);
    }

    void on_update(float dt) override {
        Scene::on_update(dt);

        auto input = window->input;
        CharacterAnimState new_state = IDLE;

        if (input->is_button_down(BUTTON_A)) {
            new_state = JUMPING;
        } else if (input->is_axis_active(AXIS_LEFT_X) ||
                   input->is_axis_active(AXIS_LEFT_Y)) {
            new_state = RUNNING;
        }

        if (new_state != current_state_) {
            set_anim_state(new_state);
        }

        // Flip based on horizontal movement
        float x_axis = input->get_axis_value(AXIS_LEFT_X);
        if (x_axis > 0.1f) {
            sprite_->flip_horizontally(false);
        } else if (x_axis < -0.1f) {
            sprite_->flip_horizontally(true);
        }

        // Move the sprite
        Vec3 pos = sprite_->transform->position();
        pos.x += input->get_axis_value(AXIS_LEFT_X) * 2.0f * dt;
        pos.y -= input->get_axis_value(AXIS_LEFT_Y) * 2.0f * dt;
        sprite_->transform->set_position(pos);
    }

private:
    void set_anim_state(CharacterAnimState state) {
        current_state_ = state;

        switch (state) {
            case IDLE:
                sprite_->animations->play_animation("idle");
                break;
            case RUNNING:
                sprite_->animations->play_animation("run");
                break;
            case JUMPING:
                sprite_->animations->play_animation("jump");
                sprite_->animations->queue_next_animation("idle");
                break;
        }

        S_DEBUG("Animation state: {}", static_cast<int>(state));
    }

    CharacterAnimState current_state_ = IDLE;
    Sprite* sprite_ = nullptr;
    Camera2D* camera_ = nullptr;
};
```

### Key Takeaways

1. **Create a `Sprite` node** and call `set_spritesheet(texture, frame_width, frame_height, attrs)` to configure the sheet.
2. **Define animations with `add_animation(name, start, end, fps)`** -- frame indices are 0-based and count left-to-right, top-to-bottom.
3. **Play animations via `sprite->animations->play_animation(name)`** -- the `animations` property is a `KeyFrameAnimationState`.
4. **Queue next animations** with `queue_next_animation(name)` for sequencing.
5. **Render dimensions are in world units** -- they control how large the sprite appears on screen, independent of the pixel dimensions of the texture frames.
6. **Use `flip_horizontally()`** to mirror the sprite for directional movement without duplicating animation frames.
7. **Animation updates are automatic** -- `Sprite::on_update()` advances the animation state each frame.

---

## Related Documentation

- [Skeleton Animation](skeleton-animation.md) -- 3D rigged character animation
- [Animation Controller](animation-controller.md) -- Playing and blending skeletal animations
- [Animation System Overview](overview.md) -- High-level overview of all animation systems
- [Sprites](../rendering/sprites.md) -- 2D sprite rendering details
- [KeyFrameAnimated](../../api/animation.md) -- API reference for keyframe animation base classes
