# Animation Controller

This document covers the `AnimationController` class in Simulant, including playing, blending, queueing, and managing skeletal animations.

---

## Table of Contents

1. [Overview](#1-overview)
2. [Getting the AnimationController](#2-getting-the-animationcontroller)
3. [Playing Animations](#3-playing-animations)
4. [Queueing Animations](#4-queueing-animations)
5. [Playback Control](#5-playback-control)
6. [Animation Data and Channels](#6-animation-data-and-channels)
7. [Target Meshes](#7-target-meshes)
8. [Implementing State Machines](#8-implementing-state-machines)
9. [How the Update Loop Works](#9-how-the-update-loop-works)
10. [Complete Example](#10-complete-example)

---

## 1. Overview

The `AnimationController` is a `StageNode` that drives skeletal animations. It is automatically created when you load a GLTF file containing animation data, and it manages playback, looping, queueing, and skinning updates.

Header: `simulant/nodes/animation_controller.h`

### What the Controller Does

- **Stores animations** as collections of channels, each targeting a node in the scene hierarchy
- **Interpolates keyframe data** (translation, rotation, scale) over time
- **Updates mesh skinning** each frame for skinned meshes
- **Manages playback state** (playing, paused, loop count, speed)
- **Supports animation queuing** for sequencing animations

---

## 2. Getting the AnimationController

When you instantiate a prefab loaded from a GLTF file, use `find_mixin<AnimationController>()` to retrieve the controller:

```cpp
auto prefab = assets->load_prefab("models/character.glb");
auto instance = create_child<smlt::PrefabInstance>(prefab);

AnimationController* controller = instance->find_mixin<AnimationController>();
```

If the GLTF file contains no animations, `find_mixin<>()` will return `nullptr`.

> **Note:** `find_mixin<>()` searches the prefab hierarchy for the first node that is (or contains) an `AnimationController`. This works because the GLTF loader attaches the controller to the appropriate node during import.

---

## 3. Playing Animations

### Listing Available Animations

```cpp
std::vector<std::string> names = controller->animation_names();
// Example: {"idle", "walk", "run", "jump"}

for (const auto& name : names) {
    S_DEBUG("Animation: {}", name);
}
```

### Playing an Animation

```cpp
// Play once
controller->play("jump");

// Loop forever
controller->play("idle", ANIMATION_LOOP_FOREVER);

// Loop a specific number of times
controller->play("walk", 3);   // Loops 3 times then stops
```

The `play()` method signature:

```cpp
bool play(const std::string& animation, int32_t loop_count = 1);
```

Parameters:
- `animation` -- The name of the animation (must match a name returned by `animation_names()`)
- `loop_count` -- Number of loops. Use `ANIMATION_LOOP_FOREVER` (`-1`) for infinite looping. Default is `1` (plays once).

Returns `true` if the animation was found and started, `false` otherwise.

### Loop Count Behavior

| `loop_count` | Behavior |
|--------------|----------|
| `1` (default) | Plays once, then stops |
| `N > 1` | Plays N times, then stops |
| `0` | Plays once, then processes the queue |
| `ANIMATION_LOOP_FOREVER` (`-1`) | Loops indefinitely |

> **Important:** Calling `play()` clears the animation queue. To sequence animations, call `play()` first, then `queue()`.

---

## 4. Queueing Animations

Queue an animation to play automatically after the current animation finishes:

```cpp
controller->play("run", 1);     // Play once
controller->queue("walk");      // Then play walk
controller->queue("idle");      // Then play idle
```

Each queued animation plays once. To loop a queued animation, you would need to re-queue it via an animation-finished handler (see [Implementing State Machines](#8-implementing-state-machines)).

```cpp
bool queue(const std::string& name);
```

Returns `true` if the animation was found and queued, `false` otherwise.

### Example: Attack Combo Sequence

```cpp
// Play a three-hit combo
controller->play("attack_1", 1);
controller->queue("attack_2");
controller->queue("attack_3");
controller->queue("idle", ANIMATION_LOOP_FOREVER);
```

> **Note:** The queue is FIFO. Each animation in the queue plays once before advancing to the next. When the queue empties and the current animation finishes, the controller stops.

---

## 5. Playback Control

### Pause and Resume

```cpp
// Pause the current animation
controller->pause();

// Resume playback
controller->resume();

// Check if paused
if (controller->is_paused()) {
    S_DEBUG("Animation is paused");
}
```

### Animation Speed

```cpp
// Slow motion
controller->set_animation_speed(0.25f);

// Normal speed (default)
controller->set_animation_speed(1.0f);

// Fast forward
controller->set_animation_speed(2.0f);
```

The speed multiplier affects how fast `time_` advances during `on_update()`. A speed of `0.5f` means the animation takes twice as long; `2.0f` means it completes in half the time.

### Animation States

```cpp
enum AnimationState {
    ANIMATION_STATE_PAUSED,
    ANIMATION_STATE_PLAYING,
};
```

State transitions:

```
[any state] --pause()--> ANIMATION_STATE_PAUSED
ANIMATION_STATE_PAUSED --resume()--> ANIMATION_STATE_PLAYING
[any state] --play("name")--> ANIMATION_STATE_PLAYING (resets time to 0)
```

---

## 6. Animation Data and Channels

### Internal Structure

Each animation stored in the controller consists of one or more **Channels**:

```cpp
struct Channel {
    FindResult<StageNode> target;       // The node to animate
    AnimationInterpolation interpolation; // LINEAR, STEP, or CUBIC_SPLINE
    AnimationDataPtr data;               // Keyframe time/value data
    AnimationPath path;                  // TRANSLATION, ROTATION, SCALE, or WEIGHTS
};

struct Animation {
    LimitedString<64> name;
    std::vector<Channel> channels;
};
```

### Animation Paths

| Path | What it Animates | Value Type |
|------|-----------------|------------|
| `ANIMATION_PATH_TRANSLATION` | Node position | `Vec3` |
| `ANIMATION_PATH_ROTATION` | Node rotation | `Quaternion` |
| `ANIMATION_PATH_SCALE` | Node scale | `Vec3` |
| `ANIMATION_PATH_WEIGHTS` | Morph target weights | `float` (not yet implemented) |

### AnimationData

`AnimationData` stores keyframe timestamps and values:

```cpp
class AnimationData {
    float max_time() const;
    float min_time() const;

    std::pair<std::size_t, std::size_t> find_times_indices(float t) const;

    template<typename T>
    T interpolated_value(AnimationInterpolation i, float t);

    bool finished(float t) const;
};
```

The `interpolated_value<T>()` method performs linear interpolation between keyframe values at time `t`. Currently, only `LINEAR` interpolation is used regardless of the `AnimationInterpolation` enum value loaded from GLTF.

---

## 7. Target Meshes

You can associate additional meshes with the controller so their skinning is updated during animation:

```cpp
bool add_target_mesh(const MeshPtr& target_mesh);
```

This is useful when a character has multiple mesh parts (e.g., a separate weapon mesh) that need to be skinned by the same animation:

```cpp
auto body_mesh = assets->load_mesh("models/character_body.glb");
auto weapon_mesh = assets->load_mesh("models/sword.glb");

controller->add_target_mesh(body_mesh);
controller->add_target_mesh(weapon_mesh);
```

When an animation plays, the controller calls `update_skinning()` on each target mesh that has `is_skinned` set to `true`. Non-skinned meshes are skipped automatically.

> **Note:** Meshes from the original prefab are automatically registered with the controller. `add_target_mesh()` is for adding meshes that were not part of the original GLTF import.

---

## 8. Implementing State Machines

Simulant does not include a built-in animation state machine. You implement transition logic in your game code:

```cpp
enum CharacterState {
    STATE_IDLE,
    STATE_WALKING,
    STATE_RUNNING,
    STATE_JUMPING,
    STATE_ATTACKING
};

class CharacterController {
public:
    void initialize(AnimationController* controller) {
        anim_controller_ = controller;
        set_state(STATE_IDLE);
    }

    void update(float dt) {
        CharacterState new_state = determine_state_from_input();

        if (new_state != current_state_) {
            transition_to(new_state);
        }

        current_state_ = new_state;
    }

private:
    void set_state(CharacterState state) {
        current_state_ = state;

        if (!anim_controller_) return;

        switch (state) {
            case STATE_IDLE:
                anim_controller_->play("idle", ANIMATION_LOOP_FOREVER);
                break;
            case STATE_WALKING:
                anim_controller_->play("walk", ANIMATION_LOOP_FOREVER);
                break;
            case STATE_RUNNING:
                anim_controller_->play("run", ANIMATION_LOOP_FOREVER);
                break;
            case STATE_JUMPING:
                anim_controller_->play("jump", 1);
                // After jump finishes, return to idle via queue
                anim_controller_->queue("idle");
                break;
            case STATE_ATTACKING:
                anim_controller_->play("attack", 1);
                anim_controller_->queue(current_state_ == STATE_RUNNING
                    ? "run" : "idle");
                break;
        }
    }

    CharacterState determine_state_from_input() {
        // ... input logic ...
        return STATE_IDLE;
    }

    CharacterState current_state_ = STATE_IDLE;
    AnimationController* anim_controller_ = nullptr;
};
```

### Detecting Animation Completion

The `AnimationController` does not emit signals when an animation finishes. To detect completion, track the animation state yourself:

```cpp
// Track the expected animation name and check if it changed
std::string last_animation_;

void check_animation_transition() {
    // The controller does not expose the current animation name publicly.
    // Workaround: track it yourself when you call play().
}
```

A practical approach is to track the current animation name in your own code and detect transitions in your update loop:

```cpp
void update(float dt) {
    if (anim_controller_ && !anim_controller_->is_paused()) {
        // If we played a one-shot animation and it finished,
        // the queue will have advanced to the next animation.
        // Monitor your state machine to detect this.
    }
}
```

---

## 9. How the Update Loop Works

The `AnimationController::on_update(float dt)` method is called every frame by the scene graph. Here is what happens:

```
1. Check state -- if paused, return immediately
2. Advance time -- time_ += dt * animation_speed_
3. For each channel in the current animation:
   a. Skip if channel target is null
   b. Interpolate the value at current time_
   c. Apply the value to the target node's transform
      - TRANSLATION -> set_translation()
      - ROTATION -> set_rotation()
      - SCALE -> set_scale_factor()
4. For each target mesh that is skinned:
   - Call mesh->update_skinning()
5. Check if animation is finished:
   a. If loop_count_ > 0: restart (time_ = 0, decrement loop_count_)
   b. If loop_count_ == 0 and queue is not empty: advance to next animation
   c. If loop_count_ == ANIMATION_LOOP_FOREVER: restart (time_ = 0)
```

### Important Details

- **Null channels are skipped**: `if(!channel.target) continue;`
- **Skinning only runs for skinned meshes**: `if(target_mesh->is_skinned && ...)`
- **Time advances even after the animation ends**: The `finished()` check happens after interpolation, so the last frame of the animation is always rendered.
- **Queue processing resets time**: When advancing to a queued animation, `time_` is reset to `0.0f`.

---

## 10. Complete Example

This example shows a full character animation setup with input-driven state transitions:

```cpp
#include "simulant/simulant.h"

using namespace smlt;

enum PlayerState {
    IDLE,
    WALKING,
    RUNNING,
    ATTACKING
};

class GameScene : public Scene {
public:
    GameScene(Window* window) : Scene(window) {}

    void on_load() override {
        // Load animated character
        auto prefab = assets->load_prefab("models/character.glb");
        character_ = create_child<PrefabInstance>(prefab);

        // Get animation controller
        anim_controller_ = character_->find_mixin<AnimationController>();

        if (!anim_controller_) {
            S_WARN("No animation controller found in prefab!");
            return;
        }

        // Debug: list all animations
        auto names = anim_controller_->animation_names();
        S_DEBUG("Loaded {} animations:", names.size());
        for (const auto& name : names) {
            S_DEBUG("  - {}", name);
        }

        // Start idle
        set_state(IDLE);

        // Camera
        camera_ = create_child<Camera3D>();
        camera_->set_perspective_projection(
            Degrees(45.0f), window->aspect_ratio(), 0.1f, 100.0f
        );
        camera_->transform->set_position(Vec3(0, 2, 5));
        camera_->transform->look_at(Vec3(0, 0, 0));

        compositor->create_layer(character_, camera_)
            ->set_clear_flags(BUFFER_CLEAR_ALL);
    }

    void on_update(float dt) override {
        Scene::on_update(dt);

        if (!anim_controller_ || anim_controller_->is_paused()) return;

        // Read input
        auto input = window->input;
        bool running = input->is_button_down(BUTTON_RIGHT_BUMPER);
        bool moving = input->is_axis_active(AXIS_LEFT_X) ||
                      input->is_axis_active(AXIS_LEFT_Y);
        bool attacking = input->is_button_down(BUTTON_A);

        // Determine state
        PlayerState new_state;
        if (attacking && current_state_ != ATTACKING) {
            new_state = ATTACKING;
        } else if (running && moving) {
            new_state = RUNNING;
        } else if (moving) {
            new_state = WALKING;
        } else {
            new_state = IDLE;
        }

        // Transition if changed
        if (new_state != current_state_) {
            set_state(new_state);
        }

        // Face movement direction
        if (moving && current_state_ != ATTACKING) {
            float x = input->get_axis_value(AXIS_LEFT_X);
            float y = input->get_axis_value(AXIS_LEFT_Y);
            if (std::abs(x) > 0.1f || std::abs(y) > 0.1f) {
                float angle = std::atan2(x, y);
                character_->transform->set_rotation(
                    Quaternion::angle_axis(Radians(angle), Vec3::up())
                );
            }
        }
    }

private:
    void set_state(PlayerState state) {
        current_state_ = state;
        if (!anim_controller_) return;

        switch (state) {
            case IDLE:
                anim_controller_->play("idle", ANIMATION_LOOP_FOREVER);
                break;
            case WALKING:
                anim_controller_->play("walk", ANIMATION_LOOP_FOREVER);
                break;
            case RUNNING:
                anim_controller_->play("run", ANIMATION_LOOP_FOREVER);
                break;
            case ATTACKING:
                anim_controller_->play("attack", 1);
                anim_controller_->queue("idle");
                break;
        }

        S_DEBUG("State changed to: {}", static_cast<int>(state));
    }

    PlayerState current_state_ = IDLE;
    PrefabInstance* character_ = nullptr;
    AnimationController* anim_controller_ = nullptr;
    Camera3D* camera_ = nullptr;
};
```

### Key Takeaways

1. **Get the controller via `find_mixin<AnimationController>()`** after loading a prefab.
2. **Use `animation_names()` for debugging** -- it shows what animations the GLTF file contains.
3. **`play(name, loop_count)`** starts an animation. Use `ANIMATION_LOOP_FOREVER` for looping states.
4. **`queue(name)`** sequences animations -- the queued animation plays when the current one finishes.
5. **`pause()` / `resume()` / `set_animation_speed()`** control playback at runtime.
6. **Implement state machines in your own code** -- the controller provides playback primitives; you decide when to transition.

---

## Related Documentation

- [Skeleton Animation](skeleton-animation.md) -- Skeletons, joints, rigs, and vertex skinning
- [Sprite Animation](sprite-animation.md) -- 2D sprite sheet animation
- [Animation System Overview](overview.md) -- High-level overview of all animation systems
- [Prefabs](../assets/prefabs.md) -- Loading animated models from GLTF files
- [Stage Nodes](../core-concepts/stage-nodes.md) -- The scene graph hierarchy
