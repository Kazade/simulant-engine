# Animation System Overview

This document covers animation in Simulant, including skeleton animation, the AnimationController, sprite animation, loading animations from GLTF/GLB files, and manual rig manipulation.

---

## Table of Contents

1. [Overview of Animation in Simulant](#1-overview-of-animation-in-simulant)
2. [Skeleton Animation](#2-skeleton-animation-rigs-skeletons-joints)
3. [AnimationController](#3-animationcontroller---playing-and-blending-animations)
4. [Loading Animations from GLTF/GLB Files](#4-loading-animations-from-gltfglb-files)
5. [Animation States and Transitions](#5-animation-states-and-transitions)
6. [Manipulating Rigs Manually](#6-manipulating-rigs-manually-look-at-ik-like-behaviors)
7. [Sprite Animation](#7-sprite-animation-2d-sprite-sheets)
8. [Animation Events and Callbacks](#8-animation-events-and-callbacks)
9. [Performance Considerations](#9-performance-considerations)
10. [Complete Code Example](#10-complete-code-example-character-animation)

---

## 1. Overview of Animation in Simulant

Simulant provides two complementary animation systems:

| System | Use Case |
|--------|----------|
| **Skeleton Animation** | 3D rigged characters with skeletal animation (bones, joints, skinning) |
| **Keyframe Animation** | 2D sprite sheets, frame-based animations |

Both systems are driven by the scene graph update loop. Every frame, the engine calls `on_update(float dt)` on all nodes, and animation state is advanced based on the delta time.

The core animation classes live in the `smlt` namespace:

- **`AnimationController`** -- A `StageNode` that plays and blends skeletal animations loaded from GLTF files. Found in `simulant/nodes/animation_controller.h`.
- **`KeyFrameAnimated`** / **`KeyFrameAnimationState`** -- Base classes for keyframe-based animation. Found in `simulant/animation.h`.
- **`Rig`** / **`RigJoint`** -- Runtime instances of a skeleton that can be manipulated per-frame. Found in `simulant/assets/meshes/rig.h`.
- **`Skeleton`** / **`Joint`** -- The definition of a skeleton (bone structure and vertex weights). Found in `simulant/assets/meshes/skeleton.h`.
- **`Sprite`** -- A `StageNode` for 2D sprite sheet animation. Found in `simulant/nodes/sprite.h`.

### How Animation Fits into the Engine

Animations are typically loaded alongside meshes via the [Prefab system](../assets/prefabs.md). A prefab (loaded from a GLTF/GLB file) contains a hierarchy of nodes, meshes, materials, and animation channels. When you instantiate a prefab, the engine creates an `AnimationController` node that you use to play animations.

See also: [Actors](../core-concepts/actors.md), [Meshes](../rendering/meshes.md), [Prefabs](../assets/prefabs.md).

---

## 2. Skeleton Animation (Rigs, Skeletons, Joints)

Skeleton animation in Simulant works by deforming a mesh based on the positions of joints (bones). A **Skeleton** defines the bone structure, and a **Rig** is a runtime instance of that skeleton whose joints can be rotated and translated each frame.

### Key Concepts

- **Skeleton** -- Stores the rest-pose bone structure and vertex-to-bone weight data. It is loaded from a mesh file and is shared among all instances.
- **Rig** -- A per-instance copy of the skeleton's joint hierarchy. You manipulate the Rig to pose the character.
- **Joint** -- A single bone in the hierarchy. Each joint has a rotation and translation relative to its parent.
- **Bone** -- A link between two joints. Created via `Joint::link_to()`.
- **SkeletalFrameUnpacker** -- Interpolates between keyframes and updates vertex positions on the mesh.

### Skeleton Structure

A skeleton consists of joints arranged in a hierarchy. Each joint has:

```
Joint
  - name           // Human-readable name (e.g., "LeftArm")
  - id             // Numeric index (0 = root)
  - parent         // Pointer to parent joint
  - rotation       // Relative rotation (Quaternion)
  - translation    // Relative translation (Vec3)
  - absolute_rotation    // World-space rotation
  - absolute_translation // World-space position
```

### Vertex Skinning

Each vertex in a skinned mesh stores up to 4 joint indices and weights:

```cpp
struct SkeletonVertex {
    int32_t joints[MAX_JOINTS_PER_VERTEX] = {-1, -1, -1, -1};
    float weights[MAX_JOINTS_PER_VERTEX] = {0, 0, 0, 0};
};
```

Constants:
- `MAX_JOINTS_PER_VERTEX` = 4
- `MAX_JOINTS_PER_MESH` = 64

### Creating a Skeleton and Rig Programmatically

```cpp
// Create a mesh with skeletal animation support
auto mesh = assets->create_mesh(smlt::VertexSpecification::DEFAULT_SKINNED);

// The mesh has a Skeleton attached
Skeleton* skeleton = mesh->skeleton.get();

// Joints are pre-allocated. Access them by index:
Joint* root = skeleton->joint(0);
root->set_name("Root");

Joint* spine = skeleton->joint(1);
spine->set_name("Spine");

// Link joints to form a bone hierarchy
root->link_to(spine);

// Attach vertices to joints with weights
skeleton->attach_vertex_to_joint(0, vertex_index, 0.8f);
skeleton->attach_vertex_to_joint(1, vertex_index, 0.2f);

// Create a Rig (runtime instance) for this skeleton
auto rig = std::make_unique<Rig>(skeleton);

// Manipulate the rig
RigJoint* spineJoint = rig->find_joint("Spine");
if (spineJoint) {
    spineJoint->rotate_to(Quaternion::angle_axis(Degrees(30), Vec3::up()));
}

// Recalculate world-space transforms
rig->recalc_absolute_transformations();
```

> **Note:** In practice, you will rarely create skeletons manually. They are loaded automatically from GLTF/GLB files. See [Loading Animations from GLTF/GLB Files](#4-loading-animations-from-gltfglb-files) below.

---

## 3. AnimationController - Playing and Blending Animations

The `AnimationController` is a `StageNode` that manages skeletal animations. It is automatically created when you load a GLTF file with animations.

### Finding the AnimationController

When you load a prefab from a GLTF file, use `find_mixin<AnimationController>()` to get the controller:

```cpp
auto prefab = assets->load_prefab("models/character.glb");
auto prefab_instance = create_child<smlt::PrefabInstance>(prefab);

// Get the animation controller
auto anim_controller = prefab_instance->find_mixin<AnimationController>();
```

### Playing Animations

```cpp
// Get all animation names
std::vector<std::string> animations = anim_controller->animation_names();
// Example: {"idle", "walk", "run", "jump"}

// Play an animation (plays once)
anim_controller->play("walk");

// Play an animation with looping
anim_controller->play("idle", ANIMATION_LOOP_FOREVER);  // Loops indefinitely
anim_controller->play("walk", 3);                        // Loops 3 times
```

The constant `ANIMATION_LOOP_FOREVER` is defined as `-1`.

### Queueing Animations

Queue an animation to play after the current one finishes:

```cpp
anim_controller->play("walk");
anim_controller->queue("idle");  // Plays "idle" when "walk" completes
```

You can queue multiple animations:

```cpp
anim_controller->play("run");
anim_controller->queue("walk");
anim_controller->queue("idle");
```

### Playback Control

```cpp
// Pause the current animation
anim_controller->pause();

// Resume playback
anim_controller->resume();

// Check if paused
if (anim_controller->is_paused()) {
    // ...
}

// Adjust playback speed
anim_controller->set_animation_speed(0.5f);  // Half speed
anim_controller->set_animation_speed(2.0f);  // Double speed
anim_controller->set_animation_speed(1.0f);  // Normal speed (default)
```

### How AnimationController Works Internally

The controller stores animations as collections of **Channels**. Each channel targets a specific node in the scene hierarchy and animates one of these paths:

| Path Constant | What it Animates |
|---------------|-----------------|
| `ANIMATION_PATH_TRANSLATION` | Node position |
| `ANIMATION_PATH_ROTATION` | Node rotation (Quaternion) |
| `ANIMATION_PATH_SCALE` | Node scale factor |
| `ANIMATION_PATH_WEIGHTS` | Morph target weights (not yet implemented) |

Each channel uses **keyframe data** with timestamps and values. Interpolation between keyframes is linear by default, with support for STEP and CUBIC_SPLINE interpolation loaded from GLTF files.

When the mesh is skinned (`mesh->is_skinned` is true), the controller calls `mesh->update_skinning()` each frame to update vertex positions based on the current rig pose.

### Target Meshes

You can associate additional meshes with the controller:

```cpp
anim_controller->add_target_mesh(mesh);
```

These meshes will have their skinning updated when the animation plays.

---

## 4. Loading Animations from GLTF/GLB Files

Simulant uses the GLTF 2.0 format as its primary asset format for animated models. Both `.gltf` (JSON) and `.glb` (binary) files are supported.

### Loading a Prefab with Animations

```cpp
// Load the prefab (this parses the GLTF file)
auto prefab = assets->load_prefab("models/character.glb");

// Instantiate it in the scene
auto instance = create_child<smlt::PrefabInstance>(prefab);

// Get the animation controller
auto controller = instance->find_mixin<AnimationController>();

// List and play animations
auto names = controller->animation_names();
for (const auto& name : names) {
    S_DEBUG("Found animation: {}", name);
}

if (!names.empty()) {
    controller->play(names[0], ANIMATION_LOOP_FOREVER);
}
```

### What the GLTF Loader Extracts

When loading a GLTF file, Simulant extracts:

- **Meshes** with positions, normals, texture coordinates
- **Skins** -- joint indices and vertex weights
- **Skeleton hierarchy** -- joint names and parent-child relationships
- **Inverse bind matrices** -- for skinning calculations
- **Animations** -- keyframe data for translation, rotation, and scale of each node
- **Materials** -- PBR materials with base color, metallic, roughness, normal maps
- **Textures** -- embedded or external textures

### GLTF Animation Structure in Simulant

Each GLTF animation becomes an `Animation` in the `AnimationController`. The animation contains one or more **Channels**, each targeting a specific node and path:

```
Animation "Walk"
  |-- Channel: target=node_5, path=TRANSLATION, interpolation=LINEAR
  |-- Channel: target=node_5, path=ROTATION, interpolation=LINEAR
  |-- Channel: target=node_8, path=TRANSLATION, interpolation=LINEAR
  |-- Channel: target=node_8, path=ROTATION, interpolation=LINEAR
  ...
```

### Skeleton Root Node

The GLTF loader identifies the skeleton root node from the `skin.skeleton` property. This determines the root of the joint hierarchy and is used for centering the model.

### Sample GLTF Files

Simulant includes sample GLTF files from the Khronos glTF-Sample-Assets repository:

```cpp
auto prefab = assets->load_prefab("assets/samples/khronos/RiggedSimple.glb");
auto prefab = assets->load_prefab("assets/samples/character-a.glb");
```

---

## 5. Animation States and Transitions

### Animation States

The `AnimationController` has two states:

```cpp
enum AnimationState {
    ANIMATION_STATE_PAUSED,
    ANIMATION_STATE_PLAYING,
};
```

State transitions are controlled through methods:

```cpp
// Transitions: any -> PAUSED
controller->pause();

// Transitions: PAUSED -> PLAYING
controller->resume();

// Transitions: any -> PLAYING (with new animation)
controller->play("new_animation");
```

### Implementing Animation State Machines

Simulant does not include a built-in animation state machine. You should implement state logic in your game code:

```cpp
enum CharacterState {
    STATE_IDLE,
    STATE_WALKING,
    STATE_RUNNING,
    STATE_JUMPING
};

class CharacterController {
public:
    void update(float dt) {
        CharacterState new_state = determine_state();

        if (new_state != current_state_) {
            transition_to(new_state);
        }

        current_state_ = new_state;
    }

private:
    void transition_to(CharacterState state) {
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
                anim_controller_->play("jump", 1);  // Play once
                break;
        }
    }

    CharacterState current_state_ = STATE_IDLE;
    AnimationController* anim_controller_ = nullptr;
};
```

### Crossfading Between Animations

The current `AnimationController` does not support automatic crossfading (blending between two animations). To implement smooth transitions, you would need to interpolate joint rotations in the Rig manually during the transition period:

```cpp
// Manual crossfade approach (advanced)
// Store the previous rig pose, blend with new pose over N frames
// This requires direct access to Rig joints and is beyond the built-in API
```

---

## 6. Manipulating Rigs Manually (Look-At, IK-like Behaviors)

You can manipulate joints in a Rig directly to create effects like look-at behavior or simple inverse kinematics.

### Accessing Joints on a Running Character

When a prefab is loaded, the Rig is not directly exposed. However, you can manipulate joint transforms through the scene node hierarchy. Each joint in the skeleton corresponds to a node in the prefab hierarchy, and the AnimationController animates those nodes' transforms.

### Look-At for a Joint

To make a character's head or eyes track a target, use `Transform::look_at()` on the appropriate node:

```cpp
// Find the head joint node in the prefab hierarchy
auto head_node = prefab_instance->find_child_by_name("Head");

if (head_node) {
    // Make the head look at the camera position
    head_node->transform->look_at(camera->transform->position());
}
```

The `look_at` method calculates the rotation needed to orient the node toward a target point:

```cpp
void Transform::look_at(const Vec3& target, const Vec3& up = Vec3::up());
```

### Manual Joint Manipulation

You can rotate and translate joints directly:

```cpp
// Find a node by name
auto left_arm = prefab_instance->find_child_by_name("LeftArm");
if (left_arm) {
    // Rotate the arm
    auto rotation = Quaternion::angle_axis(Degrees(45), Vec3::right());
    left_arm->transform->set_rotation(rotation);

    // Or set position
    left_arm->transform->set_position(Vec3(0.5f, 1.0f, 0.0f));
}
```

### Simple IK-like Behavior

For a two-bone IK solver (e.g., arm or leg), you can compute joint angles manually:

```cpp
void solve_two_bone_ik(
    StageNode* shoulder,
    StageNode* elbow,
    StageNode* hand,
    const Vec3& target
) {
    if (!shoulder || !elbow || !hand) return;

    Vec3 shoulder_pos = shoulder->transform->position();
    float upper_length = (elbow->transform->position() - shoulder_pos).length();
    float lower_length = (hand->transform->position() - elbow->transform->position()).length();

    Vec3 to_target = target - shoulder_pos;
    float target_dist = to_target.length();

    // Clamp target to maximum reach
    float max_reach = upper_length + lower_length;
    if (target_dist > max_reach) {
        to_target = to_target.normalized() * max_reach;
        target_dist = max_reach;
    }

    // Calculate shoulder rotation (look at target)
    if (target_dist > 0.001f) {
        shoulder->transform->look_at(shoulder_pos + to_target);
    }

    // Calculate elbow bend angle using law of cosines
    float cos_angle = (upper_length * upper_length + target_dist * target_dist - lower_length * lower_length)
                      / (2.0f * upper_length * target_dist);
    cos_angle = clamp(cos_angle, -1.0f, 1.0f);
    float elbow_angle = std::acos(cos_angle);

    // Apply elbow rotation (bend)
    // This depends on your bone orientation; adjust axis as needed
    auto elbow_rot = Quaternion::angle_axis(Radians(elbow_angle), Vec3::right());
    elbow->transform->set_rotation(elbow_rot);
}
```

### Debugging Joint Positions

Enable debug drawing to visualize the skeleton:

```cpp
// The SkeletalFrameUnpacker draws joint lines in debug mode
// Pass a Debug node to unpack_frame to see joint positions
```

---

## 7. Sprite Animation (2D Sprite Sheets)

For 2D animation, Simulant provides the `Sprite` node. It works by updating texture coordinates on a quad to display different frames from a sprite sheet.

### Creating a Sprite

```cpp
auto sprite = scene->create_node<Sprite>();
```

### Setting Up a Sprite Sheet

```cpp
// Load the sprite sheet texture
auto texture = assets->load_texture("textures/character_sheet.png");

// Configure the sprite sheet
// frame_width/frame_height = size of each frame in pixels
SpritesheetAttrs attrs;
attrs.margin = 0;       // Margin around the entire sheet
attrs.spacing = 2;      // Spacing between frames
attrs.padding_horizontal = 0;
attrs.padding_vertical = 0;

sprite->set_spritesheet(texture, 64, 64, attrs);

// Set the render size in world units
sprite->set_render_dimensions(1.0f, 1.0f);

// Optional: set alpha transparency
sprite->set_alpha(1.0f);

// Optional: flip the sprite
sprite->flip_horizontally(true);
sprite->flip_vertically(false);
```

### Defining Animations

The `Sprite` class inherits from `KeyFrameAnimated`. Define animations by specifying frame ranges:

```cpp
// Add animation named "run" using frames 0-7 at 10 fps
sprite->add_animation("run", 0, 7, 10.0f);

// Add animation using default FPS
sprite->add_animation("idle", 8, 15);

// Set the default FPS (used when not specified)
sprite->set_default_fps(12.0f);
```

### Playing Sprite Animations

The sprite's animation state is accessed through the `animations` property:

```cpp
// Play an animation
sprite->animations->play_animation("run");

// Queue another to play after
sprite->animations->queue_next_animation("idle");

// Play the first defined animation
sprite->animations->play_first_animation();

// Update is automatic via Sprite::on_update(dt)
```

### How Sprite Animation Works

The sprite creates a quad mesh internally. When an animation frame changes, it recalculates the UV coordinates to display the correct region of the sprite sheet:

```
Sprite sheet layout (frames laid out left-to-right, top-to-bottom):

+------+------+------+
|  0   |  1   |  2   |
+------+------+------+
|  3   |  4   |  5   |
+------+------+------+

Frame N is located at:
  column = N % frames_per_row
  row    = N / frames_per_row
```

The texture coordinates are recalculated each frame, accounting for margin, spacing, and padding.

---

## 8. Animation Events and Callbacks

### KeyFrameAnimated Signal

The `KeyFrameAnimated` class provides a signal that fires when an animation is added:

```cpp
signal_animation_added().connect([](KeyFrameAnimated* animatable, const std::string& name) {
    S_DEBUG("Animation added: {}", name);
});
```

### AnimationUpdatedCallback

When creating a `KeyFrameAnimationState`, you provide a callback that is called each frame when the animation updates:

```cpp
auto anim_state = std::make_shared<KeyFrameAnimationState>(
    sprite,
    [this](int32_t current_frame, int32_t next_frame, float interp) {
        // Called every frame the animation updates
        // current_frame: the current keyframe index
        // next_frame: the next keyframe index
        // interp: interpolation factor (0.0 - 1.0) between current and next
        this->on_animation_frame_changed(current_frame, next_frame, interp);
    }
);
```

For sprites, this callback is used internally to update texture coordinates.

### Detecting Animation Completion

The `AnimationController` does not have a built-in "animation finished" signal. To detect completion, track the animation state manually:

```cpp
class MyGameScene : public Scene {
public:
    void on_update(float dt) override {
        Scene::on_update(dt);

        // Check if the queued animation has started (meaning the previous one ended)
        // You can track this by monitoring the current animation name
    }

private:
    void on_animation_finished(const std::string& animation_name) {
        S_DEBUG("Animation finished: {}", animation_name);

        if (animation_name == "jump") {
            anim_controller_->play("idle", ANIMATION_LOOP_FOREVER);
        }
    }
};
```

For keyframe animation, you can check when frames reach the end:

```cpp
// In your update loop
void update(float dt) {
    auto state = sprite->animations.get();
    if (state) {
        uint32_t frame = state->current_frame();
        float interp = state->interp();

        // If we're on the last frame with full interpolation, animation is about to loop/complete
        // (The exact logic depends on whether you want to detect loop vs. completion)
    }
}
```

---

## 9. Performance Considerations

### CPU Skinning Cost

Skeletal animation in Simulant is done on the CPU. The `SkeletalFrameUnpacker::unpack_frame()` method iterates over every vertex and applies joint transformations. This means:

- **Vertex count matters**: More vertices = more CPU work per frame
- **Joint count is bounded**: Maximum 64 joints per mesh, 4 joints per vertex
- **Skinned meshes cost more**: Non-skinned meshes skip the skinning update entirely

The engine checks `mesh->is_skinned` before updating skinning, so non-animated meshes are not penalized.

### Frame Rate Throttling

The `KeyFrameAnimationState` uses a `Throttle` capped at 60 FPS to prevent excessive updates:

```cpp
Throttle throttle_ = Throttle(60);
```

This means keyframe animations will not exceed 60 updates per second regardless of frame rate.

### AnimationController Optimization

- Channels with null targets are skipped (`if(!channel.target) continue;`)
- Skinning update only runs for skinned meshes
- The controller stops processing when the current animation index is out of range

### Memory Considerations

- **Skeleton data is shared**: The `Skeleton` is stored on the `Mesh` and shared across instances
- **Rig is per-instance**: Each animated character needs its own `Rig`
- **AnimationData stores times and values**: Both are kept in memory for the lifetime of the prefab

### Best Practices

1. **Keep joint counts low**: Stay well under the 64-joint maximum
2. **Use LOD for distant characters**: Switch to simpler meshes or disable animation
3. **Batch static characters**: If a character doesn't animate, avoid unnecessary skinning updates
4. **Use sprite sheets efficiently**: Pack multiple animations into one sheet to reduce texture swaps
5. **Preload animations**: Load GLTF files during loading screens, not during gameplay

### Platform-Specific Notes

On resource-constrained platforms (Dreamcast, PSP), consider:

- Reducing vertex count on animated meshes
- Using fewer simultaneous animated characters
- Preferring sprite animation over skeletal animation for 2D characters

---

## 10. Complete Code Example: Character Animation

This example demonstrates loading an animated character from a GLTF file and controlling its animations based on player input.

```cpp
#include "simulant/simulant.h"

using namespace smlt;

enum class PlayerState {
    IDLE,
    WALKING,
    RUNNING
};

class GameScene : public Scene {
public:
    GameScene(Window* window) : Scene(window) {}

    void on_load() override {
        // Load the animated character from GLTF
        auto prefab = assets->load_prefab("models/character.glb");
        character_ = create_child<PrefabInstance>(prefab);
        character_->transform->set_position(Vec3(0, 0, -5.0f));

        // Get the animation controller
        anim_controller_ = character_->find_mixin<AnimationController>();

        if (anim_controller_) {
            // List all available animations
            auto animations = anim_controller_->animation_names();
            S_DEBUG("Loaded {} animations:", animations.size());
            for (const auto& name : animations) {
                S_DEBUG("  - {}", name);
            }

            // Start with idle animation
            set_state(PlayerState::IDLE);
        }

        // Set up camera
        auto camera = create_child<Camera3D>({
            {"znear",  0.1f},
            {"zfar",   100.0f},
            {"aspect", window->aspect_ratio()},
            {"yfov",   45.0f}
        });
        camera->set_perspective_projection(
            Degrees(45.0), window->aspect_ratio(), 0.1f, 100.0f
        );
        camera->transform->set_position(Vec3(0, 2, 3));
        camera->transform->look_at(Vec3(0, 0, -5));

        // Create render layer
        auto layer = compositor->create_layer(character_, camera);
        layer->set_clear_flags(BUFFER_CLEAR_ALL);
        layer->viewport->set_color(Color::gray());

        // Enable debug drawing to see skeleton
        debug_ = scene->debug();
    }

    void on_update(float dt) override {
        Scene::on_update(dt);

        if (!anim_controller_ || anim_controller_->is_paused()) return;

        // Simple state machine based on input
        auto input = window->input;

        bool is_running = input->is_button_down(BUTTON_RIGHT_BUMPER);
        bool is_moving = input->is_axis_active(AXIS_LEFT_X) ||
                         input->is_axis_active(AXIS_LEFT_Y);

        PlayerState new_state;
        if (is_running && is_moving) {
            new_state = PlayerState::RUNNING;
        } else if (is_moving) {
            new_state = PlayerState::WALKING;
        } else {
            new_state = PlayerState::IDLE;
        }

        if (new_state != current_state_) {
            set_state(new_state);
        }

        // Make character face movement direction
        if (is_moving) {
            float x = input->get_axis_value(AXIS_LEFT_X);
            float y = input->get_axis_value(AXIS_LEFT_Y);
            if (std::abs(x) > 0.1f || std::abs(y) > 0.1f) {
                float angle = std::atan2(x, y);
                auto rot = Quaternion::angle_axis(Radians(angle), Vec3::up());
                character_->transform->set_rotation(rot);
            }
        }

        // Debug: draw skeleton joints
        if (debug_) {
            // Joint visualization is handled by the SkeletalFrameUnpacker
            // in debug mode when a Debug pointer is passed
        }
    }

private:
    void set_state(PlayerState state) {
        current_state_ = state;

        if (!anim_controller_) return;

        switch (state) {
            case PlayerState::IDLE:
                anim_controller_->play("idle", ANIMATION_LOOP_FOREVER);
                break;
            case PlayerState::WALKING:
                anim_controller_->play("walk", ANIMATION_LOOP_FOREVER);
                break;
            case PlayerState::RUNNING:
                anim_controller_->play("run", ANIMATION_LOOP_FOREVER);
                break;
        }

        S_DEBUG("Character state changed to: {}", static_cast<int>(state));
    }

    PlayerState current_state_ = PlayerState::IDLE;
    PrefabInstance* character_ = nullptr;
    AnimationController* anim_controller_ = nullptr;
    Debug* debug_ = nullptr;
};

class AnimationDemo : public Application {
public:
    AnimationDemo(const AppConfig& config) : Application(config) {}

private:
    bool init() override {
        scenes->register_scene<GameScene>("game");
        scenes->activate("_loading");
        scenes->preload_in_background("game").then([this]() {
            scenes->activate("game");
        });
        return true;
    }
};

int main(int argc, char* argv[]) {
    _S_UNUSED(argc);
    _S_UNUSED(argv);

    AppConfig config;
    config.title = "Character Animation Demo";
    config.width = 1280;
    config.height = 720;
    config.fullscreen = false;
    config.log_level = LOG_LEVEL_DEBUG;

    AnimationDemo app(config);
    return app.run();
}
```

### Key Takeaways from the Example

1. **Load prefabs with `assets->load_prefab()`** -- this parses GLTF and extracts meshes, skeletons, and animations.
2. **Get the `AnimationController` via `find_mixin<>()`** -- this is how you access animation playback controls.
3. **List animations with `animation_names()`** -- useful for debugging and dynamic state machines.
4. **Play animations with `play(name, loop_count)`** -- use `ANIMATION_LOOP_FOREVER` for looping animations.
5. **Implement state machines in game code** -- the engine provides the playback primitives; you provide the logic.
6. **Combine with transforms** -- use `transform->look_at()` and `transform->set_rotation()` for facing direction.

---

## Related Documentation

- [Actors](../core-concepts/actors.md) -- Mesh-rendering entities in the scene
- [Meshes](../rendering/meshes.md) -- 3D geometry and mesh handling
- [Prefabs](../assets/prefabs.md) -- Reusable scene templates loaded from GLTF
- [Transforms](../core-concepts/transforms.md) -- Positioning, rotation, and scaling
- [Stage Nodes](../core-concepts/stage-nodes.md) -- The scene graph hierarchy
- [Sprites](../rendering/sprites.md) -- 2D sprite rendering details
