# Tutorial 5: Character Animation

In this tutorial, you will learn how to load and control animated characters in Simulant. You will load a rigged model from a GLB file, play and loop animations, and build a simple state machine to switch between idle, walking, and running animations based on input.

**Prerequisites:** [Tutorial 1 -- Basic Application](01_basic_application.md), [Tutorial 2 -- Loading Models](02_loading_models.md)

**Related documentation:** [Animation System Overview](../animation/overview.md), [Animation Controller](../animation/animation-controller.md), [Skeleton Animation](../animation/skeleton-animation.md), [Prefabs](../assets/prefabs.md).

---

## What You Will Build

By the end of this tutorial, you will have a working application that:

- Loads an animated character from a GLB file
- Lists and plays available animations
- Builds a state machine to switch animations based on input
- Supports looping, queuing, and speed control
- Optionally covers 2D sprite animation

---

## Step 1: Understanding Animation in Simulant

Simulant provides two complementary animation systems:

| System | Use Case |
|--------|----------|
| **Skeleton Animation** | 3D rigged characters with skeletal animation (bones, joints, skinning) |
| **Sprite Animation** | 2D sprite sheets, frame-based animations |

For 3D characters, animations are loaded from GLTF/GLB files. The GLTF loader extracts:

- **Skeleton hierarchy** -- joint names and parent-child relationships
- **Skins** -- joint indices and vertex weights
- **Animations** -- keyframe data for translation, rotation, and scale of each node

When you instantiate a prefab from a GLB file, an `AnimationController` is automatically attached if the file contains animations.

---

## Step 2: Setting Up the Application

Start with the basic application structure:

```cpp
#include "simulant/simulant.h"

using namespace smlt;

class AnimationScene : public Scene {
public:
    AnimationScene(Window* window) : Scene(window) {}

    void on_load() override {
        // We will load our animated character here
    }
};

class AnimationDemo : public Application {
public:
    AnimationDemo(const AppConfig& config) : Application(config) {}

private:
    bool init() override {
        scenes->register_scene<AnimationScene>("main");
        scenes->activate("main");
        return true;
    }
};

int main(int argc, char* argv[]) {
    _S_UNUSED(argc);
    _S_UNUSED(argv);

    AppConfig config;
    config.title = "Animation Demo";
    config.width = 1280;
    config.height = 960;
    config.fullscreen = false;
    config.log_level = LOG_LEVEL_DEBUG;

    AnimationDemo app(config);
    return app.run(argc, argv);
}
```

---

## Step 3: Loading an Animated Character

Load the prefab from a GLB file and instantiate it:

```cpp
void on_load() override {
    // Load the animated character
    auto prefab = assets->load_prefab("models/character.glb");

    if (!prefab) {
        S_ERROR("Failed to load character prefab!");
        return;
    }

    // Instantiate -- this creates the full hierarchy including AnimationController
    character_ = create_child<PrefabInstance>(prefab);
    character_->transform->set_position(Vec3(0, 0, -5.0f));

    // Get the animation controller
    anim_controller_ = character_->find_mixin<AnimationController>();

    if (!anim_controller_) {
        S_WARN("No animations found in GLB file!");
        return;
    }

    // List all available animations (great for debugging)
    auto animations = anim_controller_->animation_names();
    S_DEBUG("Loaded {} animations:", animations.size());
    for (const auto& name : animations) {
        S_DEBUG("  - {}", name);
    }

    // Start with the first animation on loop
    if (!animations.empty()) {
        anim_controller_->play(animations[0], ANIMATION_LOOP_FOREVER);
        S_DEBUG("Playing: {}", animations[0].c_str());
    }
}
```

### What is happening here?

1. **`load_prefab()`** reads the GLB file and builds a `Prefab` with all nodes, meshes, and animation data.
2. **`create_child<PrefabInstance>()`** instantiates the prefab. The GLTF loader creates an `AnimationController` mixin automatically.
3. **`find_mixin<AnimationController>()`** retrieves the controller so you can play animations.
4. **`animation_names()`** lists all animations found in the file -- useful for debugging.
5. **`play()`** starts playback. Use `ANIMATION_LOOP_FOREVER` for looping animations.

---

## Step 4: Setting Up a Camera and Lighting

To see the animated character, add a camera and a light:

```cpp
void on_load() override {
    // ... (character loading from above) ...

    // Create a camera
    auto camera = create_child<Camera3D>({
        {"znear",  0.1f},
        {"zfar",   100.0f},
        {"aspect", window->aspect_ratio()},
        {"yfov",   45.0f}
    });

    camera->set_perspective_projection(
        Degrees(45.0),
        window->aspect_ratio(),
        0.1f,
        100.0f
    );

    camera->transform->set_position(Vec3(0, 2, 5));
    camera->transform->look_at(Vec3(0, 0, -5));

    // Create a render layer
    auto layer = compositor->create_layer(character_, camera);
    layer->set_clear_flags(BUFFER_CLEAR_ALL);
    layer->viewport->set_color(Color::gray());

    // Add a directional light
    auto light = create_child<Light>(LIGHT_DIRECTIONAL);
    light->set_color(Color(1.0f, 1.0f, 0.9f));
    light->set_direction(Vec3(-1, -1, -1).normalized());
}
```

---

## Step 5: Building an Animation State Machine

In a real game, you switch animations based on game state. Here is a simple state machine:

```cpp
enum PlayerState {
    STATE_IDLE,
    STATE_WALKING,
    STATE_RUNNING
};

class AnimationScene : public Scene {
    // ... existing members ...

private:
    PlayerState current_state_ = STATE_IDLE;

    void set_state(PlayerState state) {
        if (state == current_state_) return;
        current_state_ = state;

        if (!anim_controller_) return;

        switch (state) {
            case STATE_IDLE:
                anim_controller_->play("idle", ANIMATION_LOOP_FOREVER);
                S_DEBUG("State: Idle");
                break;
            case STATE_WALKING:
                anim_controller_->play("walk", ANIMATION_LOOP_FOREVER);
                S_DEBUG("State: Walking");
                break;
            case STATE_RUNNING:
                anim_controller_->play("run", ANIMATION_LOOP_FOREVER);
                S_DEBUG("State: Running");
                break;
        }
    }

    void on_update(float dt) override {
        Scene::on_update(dt);

        if (!anim_controller_ || anim_controller_->is_paused()) return;

        auto input = window->input;

        // Determine desired state from input
        PlayerState new_state = STATE_IDLE;

        if (input->is_axis_active(AXIS_LEFT_X) ||
            input->is_axis_active(AXIS_LEFT_Y)) {

            // Check if running (hold right bumper)
            if (input->is_button_down(BUTTON_RIGHT_BUMPER)) {
                new_state = STATE_RUNNING;
            } else {
                new_state = STATE_WALKING;
            }
        }

        // Transition if state changed
        set_state(new_state);

        // Face movement direction
        if (new_state != STATE_IDLE) {
            float x = input->get_axis_value(AXIS_LEFT_X);
            float y = input->get_axis_value(AXIS_LEFT_Y);

            if (std::abs(x) > 0.1f || std::abs(y) > 0.1f) {
                float angle = std::atan2(x, y);
                character_->transform->set_rotation(
                    Quaternion::angle_axis(Radians(angle), Vec3::UP)
                );
            }
        }
    }
};
```

---

## Step 6: Playback Control

The `AnimationController` provides several playback methods you can use at runtime:

### Pause and resume

```cpp
// Pause the current animation
anim_controller_->pause();

// Resume playback
anim_controller_->resume();

// Check if paused
if (anim_controller_->is_paused()) {
    S_DEBUG("Animation is paused");
}
```

### Animation speed

```cpp
// Slow motion
anim_controller_->set_animation_speed(0.5f);

// Normal speed (default)
anim_controller_->set_animation_speed(1.0f);

// Fast forward
anim_controller_->set_animation_speed(2.0f);
```

### Playing once then returning to idle

```cpp
// Play a one-shot animation, then queue idle
anim_controller_->play("jump", 1);       // Play once
anim_controller_->queue("idle");          // Return to idle after jump finishes
```

### Queueing multiple animations

```cpp
anim_controller_->play("attack_1", 1);
anim_controller_->queue("attack_2");
anim_controller_->queue("attack_3");
anim_controller_->queue("idle", ANIMATION_LOOP_FOREVER);
```

> **Important:** Calling `play()` clears the animation queue. To sequence animations, call `play()` first, then `queue()`.

---

## Step 7: Finding and Manipulating Joints

You can find specific nodes within the animated character and manipulate them. For example, making a character's head track the camera:

```cpp
void on_update(float dt) override {
    Scene::on_update(dt);

    // Make the character's head look at the camera
    auto head = character_->find_descendent_with_name("Head");
    if (head) {
        head->transform->look_at(camera_->transform->position());
    }
}
```

### Manual joint rotation

You can rotate individual joints:

```cpp
auto left_arm = character_->find_descendent_with_name("LeftArm");
if (left_arm) {
    auto rotation = Quaternion::angle_axis(Degrees(45), Vec3::RIGHT);
    left_arm->transform->set_rotation(rotation);
}
```

> **Note:** When the animation is playing, the `AnimationController` overwrites joint transforms every frame. Manual joint manipulation works best when the animation is paused or when animating joints that the current animation does not target.

---

## Step 8: 2D Sprite Animation

For 2D games, Simulant provides the `Sprite` node. Here is how to set up a sprite sheet with animations:

### Creating a Sprite

```cpp
// Load the sprite sheet texture
auto texture = assets->load_texture("textures/hero_sheet.png");

// Create the sprite
auto sprite = create_child<Sprite>();

// Configure the sprite sheet
// frame_width = 64, frame_height = 64, spacing = 2px between frames
SpritesheetAttrs attrs;
attrs.spacing = 2;

sprite->set_spritesheet(texture, 64, 64, attrs);
sprite->set_render_dimensions(1.0f, 1.0f);  // Size in world units
```

### Defining animations

```cpp
// Assuming the sprite sheet has:
// Frames 0-3: idle
// Frames 4-11: run
// Frames 12-15: jump
sprite->add_animation("idle", 0, 3, 8.0f);
sprite->add_animation("run", 4, 11, 12.0f);
sprite->add_animation("jump", 12, 15, 10.0f);

// Start with idle
sprite->animations->play_animation("idle");
```

### Playing sprite animations

```cpp
// Play a specific animation
sprite->animations->play_animation("run");

// Queue next animation
sprite->animations->queue_next_animation("idle");

// Play a sequence
sprite->animations->play_animation("jump");
sprite->animations->queue_next_animation("idle");
```

### Flipping the sprite

```cpp
// Mirror based on movement direction
if (moving_left) {
    sprite->flip_horizontally(true);
} else if (moving_right) {
    sprite->flip_horizontally(false);
}
```

### A complete sprite scene

```cpp
class SpriteScene : public Scene {
public:
    SpriteScene(Window* window) : Scene(window) {}

    void on_load() override {
        auto texture = assets->load_texture("textures/hero_sheet.png");
        if (!texture) {
            S_ERROR("Failed to load sprite sheet!");
            return;
        }

        sprite_ = create_child<Sprite>();

        SpritesheetAttrs attrs;
        attrs.spacing = 2;
        sprite_->set_spritesheet(texture, 64, 64, attrs);
        sprite_->set_render_dimensions(1.0f, 1.0f);

        sprite_->add_animation("idle", 0, 3, 8.0f);
        sprite_->add_animation("run", 4, 11, 12.0f);
        sprite_->add_animation("jump", 12, 15, 10.0f);

        sprite_->animations->play_animation("idle");

        // 2D camera
        auto camera = create_child<Camera2D>();
        camera->transform->set_position(Vec3(0, 0, 10));

        compositor->create_layer(sprite_, camera)
            ->set_clear_flags(BUFFER_CLEAR_ALL);
    }

    void on_update(float dt) override {
        Scene::on_update(dt);

        auto input = window->input;

        if (input->is_button_down(BUTTON_A)) {
            sprite_->animations->play_animation("jump");
            sprite_->animations->queue_next_animation("idle");
        } else if (input->is_axis_active(AXIS_LEFT_X)) {
            sprite_->animations->play_animation("run");

            // Flip based on direction
            float x = input->get_axis_value(AXIS_LEFT_X);
            sprite_->flip_horizontally(x < 0);
        } else {
            sprite_->animations->play_animation("idle");
        }
    }

private:
    Sprite* sprite_ = nullptr;
};
```

---

## Complete Example: 3D Character Animation

Here is the full working application:

```cpp
#include "simulant/simulant.h"

using namespace smlt;

enum PlayerState {
    STATE_IDLE,
    STATE_WALKING,
    STATE_RUNNING
};

class AnimationScene : public Scene {
public:
    AnimationScene(Window* window) : Scene(window) {}

    void on_load() override {
        // ---- Load the animated character ----
        auto prefab = assets->load_prefab("models/character.glb");

        if (!prefab) {
            S_ERROR("Failed to load character!");
            return;
        }

        character_ = create_child<PrefabInstance>(prefab);
        character_->transform->set_position(Vec3(0, 0, -5.0f));

        // ---- Get the animation controller ----
        anim_controller_ = character_->find_mixin<AnimationController>();

        if (!anim_controller_) {
            S_WARN("No animations found in character file");
        } else {
            // Debug: list all animations
            auto names = anim_controller_->animation_names();
            S_DEBUG("Loaded {} animations:", names.size());
            for (const auto& name : names) {
                S_DEBUG("  - {}", name);
            }

            // Start idle
            set_state(STATE_IDLE);
        }

        // ---- Camera ----
        camera_ = create_child<Camera3D>({
            {"znear",  0.1f},
            {"zfar",   100.0f},
            {"aspect", window->aspect_ratio()},
            {"yfov",   45.0f}
        });

        camera_->set_perspective_projection(
            Degrees(45.0),
            window->aspect_ratio(),
            0.1f,
            100.0f
        );

        camera_->transform->set_position(Vec3(0, 2, 5));
        camera_->transform->look_at(Vec3(0, 0, -5));

        // ---- Render layer ----
        auto layer = compositor->create_layer(character_, camera_);
        layer->set_clear_flags(BUFFER_CLEAR_ALL);
        layer->viewport->set_color(Color::gray());

        // ---- Light ----
        auto light = create_child<Light>(LIGHT_DIRECTIONAL);
        light->set_color(Color(1.0f, 1.0f, 0.9f));
        light->set_direction(Vec3(-1, -1, -1).normalized());
    }

    void on_update(float dt) override {
        Scene::on_update(dt);

        if (!anim_controller_ || anim_controller_->is_paused()) return;

        auto input = window->input;

        // Determine desired state
        PlayerState new_state = STATE_IDLE;

        if (input->is_axis_active(AXIS_LEFT_X) ||
            input->is_axis_active(AXIS_LEFT_Y)) {

            if (input->is_button_down(BUTTON_RIGHT_BUMPER)) {
                new_state = STATE_RUNNING;
            } else {
                new_state = STATE_WALKING;
            }
        }

        // Press J to toggle animation speed
        if (input->is_button_down(BUTTON_J)) {
            float current_speed = anim_controller_->animation_speed();
            if (current_speed >= 1.0f) {
                anim_controller_->set_animation_speed(0.5f);
                S_DEBUG("Slow motion");
            } else {
                anim_controller_->set_animation_speed(1.0f);
                S_DEBUG("Normal speed");
            }
        }

        // Press P to pause/resume
        if (input->is_button_down(BUTTON_P)) {
            if (anim_controller_->is_paused()) {
                anim_controller_->resume();
                S_DEBUG("Resumed");
            } else {
                anim_controller_->pause();
                S_DEBUG("Paused");
            }
        }

        // Transition if state changed
        if (new_state != current_state_) {
            set_state(new_state);
        }

        // Face movement direction
        if (new_state != STATE_IDLE) {
            float x = input->get_axis_value(AXIS_LEFT_X);
            float y = input->get_axis_value(AXIS_LEFT_Y);

            if (std::abs(x) > 0.1f || std::abs(y) > 0.1f) {
                float angle = std::atan2(x, y);
                character_->transform->set_rotation(
                    Quaternion::angle_axis(Radians(angle), Vec3::UP)
                );
            }
        }
    }

private:
    void set_state(PlayerState state) {
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
        }
    }

    PlayerState current_state_ = STATE_IDLE;
    PrefabInstance* character_ = nullptr;
    AnimationController* anim_controller_ = nullptr;
    Camera3D* camera_ = nullptr;
};

class AnimationDemo : public Application {
public:
    AnimationDemo(const AppConfig& config) : Application(config) {}

private:
    bool init() override {
        scenes->register_scene<AnimationScene>("main");
        scenes->activate("main");
        return true;
    }
};

int main(int argc, char* argv[]) {
    _S_UNUSED(argc);
    _S_UNUSED(argv);

    AppConfig config;
    config.title = "Animation Demo";
    config.width = 1280;
    config.height = 960;
    config.fullscreen = false;
    config.log_level = LOG_LEVEL_DEBUG;

    AnimationDemo app(config);
    return app.run(argc, argv);
}
```

---

## Best Practices

### 1. Use animation names for debugging

Always log available animations when loading a character. GLTF animation names depend on how the file was exported:

```cpp
auto names = anim_controller_->animation_names();
for (const auto& name : names) {
    S_DEBUG("  - {}", name);
}
```

### 2. Use ANIMATION_LOOP_FOREVER for looping states

Idle, walk, and run animations typically loop forever. One-shot animations (jump, attack) play once and queue a return to idle:

```cpp
anim_controller_->play("jump", 1);
anim_controller_->queue("idle");
```

### 3. Track animation state in your own code

The `AnimationController` does not emit "animation finished" signals. Implement state transitions in your game code based on input and game logic.

### 4. Export glTF with Y-up orientation

Simulant expects glTF files exported as Y-up. Configure your 3D modelling tool accordingly.

### 5. Bake IK before export

Inverse kinematics are not supported at runtime. Bake IK into keyframes before exporting your GLB file.

---

## Summary

| Concept | Key Methods |
|---------|------------|
| Load animated prefab | `assets->load_prefab("character.glb")` |
| Get animation controller | `instance->find_mixin<AnimationController>()` |
| List animations | `controller->animation_names()` |
| Play animation | `controller->play("name", ANIMATION_LOOP_FOREVER)` |
| Queue animation | `controller->queue("name")` |
| Pause/resume | `controller->pause()`, `controller->resume()` |
| Set speed | `controller->set_animation_speed(0.5f)` |
| Create sprite | `create_child<Sprite>()` |
| Set sprite sheet | `sprite->set_spritesheet(texture, frame_w, frame_h, attrs)` |
| Add sprite animation | `sprite->add_animation("name", start, end, fps)` |
| Play sprite animation | `sprite->animations->play_animation("name")` |
| Flip sprite | `sprite->flip_horizontally(true)` |

---

## Next Steps

You now know the basics of the Simulant game engine. From here, you can:

- Read the detailed guides: [Physics](../physics/overview.md), [Rendering](../rendering/lighting.md), [Scripting](../scripting/behaviours.md)
- Study the sample code in the `samples/` directory
- Explore the asset pipeline for importing models from Blender and other tools
- Build your first complete game!
