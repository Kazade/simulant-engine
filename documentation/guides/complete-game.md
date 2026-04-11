# Building a Complete Game in Simulant: A Step-by-Step Guide

This guide walks you through building a simple but complete 3D platformer game called **"Crystal Quest"** from scratch. The player collects crystals across multiple levels, avoids hazards, and tries to reach the goal platform. Along the way you will learn every major system in Simulant: scenes, actors, physics, animation, UI, audio, scene transitions, and packaging.

> **Who this is for**: Junior developers who understand basic C++ (classes, pointers, lambdas) and have followed the [Installation Guide](../getting-started/installation.md) and [Your First Game](../getting-started/first-game.md) tutorial.
>
> **Estimated time**: 2-4 hours depending on experience.
>
> **Prerequisites**: Simulant installed and working. If you haven't set up yet, see [Installation Guide](../getting-started/installation.md).

---

## Table of Contents

1. [Project Setup and Structure](#1-project-setup-and-structure)
2. [Creating the Main Menu Scene](#2-creating-the-main-menu-scene)
3. [Creating the Game Scene with a Player Character](#3-creating-the-game-scene-with-a-player-character)
4. [Adding Physics: Gravity, Collision, and Jumping](#4-adding-physics-gravity-collision-and-jumping)
5. [Loading 3D Models from Prefabs](#5-loading-3d-models-from-prefabs)
6. [Adding Animations: Idle, Walk, Jump](#6-adding-animations-idle-walk-jump)
7. [Creating a HUD: Score, Lives, Health](#7-creating-a-hud-score-lives-health)
8. [Adding Sound Effects and Music](#8-adding-sound-effects-and-music)
9. [Multiple Levels with Scene Transitions](#9-multiple-levels-with-scene-transitions)
10. [Game Over and Victory Conditions](#10-game-over-and-victory-conditions)
11. [Settings Scene: Volume Control](#11-settings-scene-volume-control)
12. [Polish and Debugging](#12-polish-and-debugging)
13. [Building and Packaging for Distribution](#13-building-and-packaging-for-distribution)

---

## 1. Project Setup and Structure

Let's start by creating a new Simulant project. Open a terminal and run:

```bash
mkdir -p ~/Projects
cd ~/Projects
simulant start crystal-quest
```

This downloads the engine, sets up the project structure, and downloads core assets. When it finishes, open the project in your IDE. The structure looks like this:

```
crystal-quest/
|-- assets/              # Game content
|-- libraries/           # Simulant library
|-- packages/            # Distribution packages
|-- sources/             # C++ source code
|   |-- main.cpp         # Entry point
|-- tests/               # Unit tests
|-- tools/               # Binary tools
|-- CMakeLists.txt       # Build configuration
|-- simulant.json        # Project metadata
```

### Organising the Source Code

Create a sensible folder structure for a multi-scene game. Inside `sources/`, create:

```
sources/
|-- main.cpp
|-- crystal_quest_app.h
|-- crystal_quest_app.cpp
|-- scenes/
|   |-- menu_scene.h
|   |-- menu_scene.cpp
|   |-- game_scene.h
|   |-- game_scene.cpp
|   |-- settings_scene.h
|   |-- settings_scene.cpp
|   |-- game_over_scene.h
|   |-- game_over_scene.cpp
|   |-- victory_scene.h
|   |-- victory_scene.cpp
|-- systems/
|   |-- player_controller.h
|   |-- player_controller.cpp
|   |-- game_state.h
|   |-- game_state.cpp
```

You can create these directories with:

```bash
cd ~/Projects/crystal-quest
mkdir -p sources/scenes sources/systems
```

### Application Entry Point

Open `sources/main.cpp` and replace the generated content:

```cpp
#include <simulant/simulant.h>
#include "crystal_quest_app.h"

int main(int argc, char* argv[]) {
    _S_UNUSED(argc);
    _S_UNUSED(argv);

    smlt::AppConfig config;
    config.title = "Crystal Quest";
    config.width = 1280;
    config.height = 720;
    config.fullscreen = false;
    config.log_level = smlt::LOG_LEVEL_DEBUG;

    CrystalQuestApp app(config);
    return app.run();
}
```

Create `sources/crystal_quest_app.h`:

```cpp
#pragma once

#include <simulant/simulant.h>

class CrystalQuestApp : public smlt::Application {
public:
    CrystalQuestApp(const smlt::AppConfig& config)
        : Application(config) {}

    bool init() override;
};
```

Create `sources/crystal_quest_app.cpp`:

```cpp
#include "crystal_quest_app.h"
#include "scenes/menu_scene.h"
#include "scenes/game_scene.h"
#include "scenes/settings_scene.h"
#include "scenes/game_over_scene.h"
#include "scenes/victory_scene.h"

bool CrystalQuestApp::init() {
    // Register all scenes
    scenes->register_scene<MenuScene>("menu");
    scenes->register_scene<GameScene>("game", 1);      // level 1
    scenes->register_scene<GameScene>("game_level_2", 2); // level 2
    scenes->register_scene<SettingsScene>("settings");
    scenes->register_scene<GameOverScene>("game_over");
    scenes->register_scene<VictoryScene>("victory");

    // Activate the menu as the starting scene
    scenes->activate("_loading");
    scenes->preload_in_background("menu").then([this]() {
        scenes->activate("menu");
    });

    return true;
}
```

The `_loading` scene is built into Simulant -- it shows a loading screen while your menu preloads in the background. The `.then()` lambda activates the menu once loading completes.

> **Key concept**: The scene named `"main"` is special -- it is the first scene the engine tries to activate. Here we use explicit activation instead, which gives us full control over the startup flow. For details see [Scene Management](../core-concepts/scene-management.md).

Let's verify the project builds at this point. Run:

```bash
simulant build
```

You'll get linker errors because the scene files don't exist yet -- that's expected. Let's fix that in the next sections.

---

## 2. Creating the Main Menu Scene

Every game needs a main menu. We'll build one with a title, a Play button, a Settings button, and a Quit button using Simulant's UI system.

### Menu Scene Header

Create `sources/scenes/menu_scene.h`:

```cpp
#pragma once

#include <simulant/simulant.h>

class MenuScene : public smlt::Scene {
public:
    MenuScene(smlt::Window* window)
        : Scene(window) {}

    void on_load() override;
    void on_activate() override;
    void on_unload() override;

private:
    smlt::CameraPtr ui_camera_;
    smlt::LayerPtr pipeline_;

    void create_ui();
};
```

### Menu Scene Implementation

Create `sources/scenes/menu_scene.cpp`:

```cpp
#include "menu_scene.h"

void MenuScene::on_load() {
    S_INFO("MenuScene loading");

    // Create a 2D camera for UI rendering
    ui_camera_ = create_child<smlt::Camera2D>();
    ui_camera_->set_orthographic_projection(
        0, window->width(), 0, window->height()
    );

    create_ui();

    // Create render pipeline for UI
    pipeline_ = compositor->create_layer(this, ui_camera_);
    pipeline_->set_clear_flags(smlt::BUFFER_CLEAR_ALL);
    pipeline_->viewport->set_color(smlt::Color(0.1f, 0.1f, 0.2f, 1.0f));
}

void MenuScene::on_activate() {
    S_INFO("MenuScene activated");
}

void MenuScene::on_unload() {
    if (pipeline_) {
        pipeline_->destroy();
        pipeline_ = nullptr;
    }
}

void MenuScene::create_ui() {
    // Use normalized coordinates for consistent positioning
    auto coord = window->coordinate_from_normalized(0.5f, 0.85f);
    int title_x = coord.x;
    int title_y = coord.y;

    // -- Title label --
    auto title = create_child<smlt::ui::Label>("Crystal Quest");
    title->set_anchor_point(0.5f, 0.5f);
    title->transform->set_position_2d(smlt::Vec2(title_x, title_y));
    title->set_text_alignment(smlt::ui::TEXT_ALIGNMENT_CENTER);
    title->set_font_size(48);

    // -- Menu frame (vertical layout container) --
    auto frame_coord = window->coordinate_from_normalized(0.5f, 0.55f);
    auto menu_frame = create_child<smlt::ui::Frame>("");
    menu_frame->set_anchor_point(0.5f, 0.5f);
    menu_frame->transform->set_position_2d(smlt::Vec2(frame_coord.x, frame_coord.y));
    menu_frame->set_padding(10);
    menu_frame->set_space_between(10);
    menu_frame->set_layout_direction(smlt::ui::LAYOUT_DIRECTION_VERTICAL);

    int button_width = 250;

    // -- Play Button --
    auto play_btn = create_child<smlt::ui::Button>("Play");
    play_btn->resize(button_width, -1);
    play_btn->set_text_alignment(smlt::ui::TEXT_ALIGNMENT_CENTER);
    play_btn->signal_activated().connect([this]() {
        scenes->activate("game");
    });
    menu_frame->pack_child(play_btn);

    // -- Settings Button --
    auto settings_btn = create_child<smlt::ui::Button>("Settings");
    settings_btn->resize(button_width, -1);
    settings_btn->set_text_alignment(smlt::ui::TEXT_ALIGNMENT_CENTER);
    settings_btn->signal_activated().connect([this]() {
        scenes->activate("settings");
    });
    menu_frame->pack_child(settings_btn);

    // -- Quit Button --
    auto quit_btn = create_child<smlt::ui::Button>("Quit");
    quit_btn->resize(button_width, -1);
    quit_btn->set_text_alignment(smlt::ui::TEXT_ALIGNMENT_CENTER);
    quit_btn->signal_activated().connect([this]() {
        app->shutdown();
    });
    menu_frame->pack_child(quit_btn);
}
```

### How the UI System Works

Simulant's UI uses a CSS-like **box model**: each widget has a border, background, foreground, and text layer. You position widgets using pixel coordinates relative to the UI camera. The `Frame` widget acts as a layout container -- it arranges its children in rows or columns.

Key methods:
- `set_anchor_point(x, y)` -- anchors the widget's position reference (0,0 = bottom-left, 1,1 = top-right, 0.5,0.5 = center)
- `transform->set_position_2d(Vec2)` -- sets the pixel position
- `resize(width, height)` -- sets the size; use `-1` to let content determine the dimension
- `signal_activated().connect(...)` -- handles button clicks

> **Further reading**: [UI System Overview](../ui/overview.md), [Widgets](../widgets.md)

Build and run to see your menu:

```bash
simulant run --rebuild
```

You should see a dark-blue screen with the title "Crystal Quest" and three centered buttons. Clicking Play will crash (GameScene doesn't exist yet) -- let's fix that next.

---

## 3. Creating the Game Scene with a Player Character

The game scene is the heart of our game. It contains the 3D world, the player character, a camera, and a render pipeline. We'll start with a simple scene: a ground plane and a player cube.

### Game Scene Header

Create `sources/scenes/game_scene.h`:

```cpp
#pragma once

#include <simulant/simulant.h>

class GameScene : public smlt::Scene {
public:
    GameScene(smlt::Window* window, int level_number)
        : Scene(window), level_number_(level_number) {}

    void on_load() override;
    void on_unload() override;
    void on_activate() override;
    void on_deactivate() override;
    void on_update(float dt) override;
    void on_fixed_update(float step) override;

    int level_number() const { return level_number_; }

private:
    int level_number_;

    // Nodes
    smlt::CameraPtr camera_;
    smlt::ActorPtr ground_;
    smlt::ActorPtr player_;

    // Pipeline
    smlt::LayerPtr pipeline_;

    // Player state
    smlt::Vec3 player_start_ = {0, 2, 0};
    float player_speed_ = 8.0f;

    void create_camera();
    void create_ground();
    void create_player();
    void create_lighting();
    void handle_input(float dt);
};
```

### Game Scene Implementation

Create `sources/scenes/game_scene.cpp`:

```cpp
#include "game_scene.h"

void GameScene::on_load() {
    S_INFO("GameScene loading level {}", level_number_);

    create_camera();
    create_lighting();
    create_ground();
    create_player();

    // Create the render pipeline
    pipeline_ = compositor->create_layer(this, camera_);
    pipeline_->set_clear_flags(smlt::BUFFER_CLEAR_ALL);
    pipeline_->viewport->set_color(smlt::Color(0.4f, 0.6f, 0.9f, 1.0f));
}

void GameScene::on_unload() {
    if (pipeline_) {
        pipeline_->destroy();
        pipeline_ = nullptr;
    }
}

void GameScene::on_activate() {
    S_INFO("GameScene activated - level {}", level_number_);

    // Reset player position
    if (player_) {
        player_->transform->set_position(player_start_);
    }
}

void GameScene::on_deactivate() {
    S_INFO("GameScene deactivated");
}

void GameScene::on_update(float dt) {
    Scene::on_update(dt);
    handle_input(dt);

    // Camera follows player
    if (player_ && camera_) {
        auto player_pos = player_->absolute_position();
        camera_->transform->set_position(
            player_pos.x,
            player_pos.y + 8.0f,
            player_pos.z - 12.0f
        );
        camera_->look_at(player_pos);
    }
}

void GameScene::on_fixed_update(float step) {
    Scene::on_fixed_update(step);
    // Physics step would go here (added in Section 4)
}

// --------------------------------------------------------------------------
// Scene construction helpers
// --------------------------------------------------------------------------

void GameScene::create_camera() {
    camera_ = create_child<smlt::Camera3D>();
    camera_->set_perspective_projection(
        smlt::Degrees(45.0f),
        window->aspect_ratio(),
        0.1f,
        1000.0f
    );
    camera_->transform->set_position(0, 8, -12);
    camera_->look_at(smlt::Vec3(0, 0, 0));
}

void GameScene::create_lighting() {
    // Ambient light
    lighting->set_ambient_light(smlt::Color(0.4f, 0.4f, 0.5f, 1.0f));

    // Directional light (sun)
    auto sun = create_child<smlt::DirectionalLight>();
    sun->set_color(smlt::Color(1.0f, 0.95f, 0.8f, 1.0f));
    sun->set_intensity(1.0f);
    sun->transform->set_rotation(
        smlt::Quaternion::angle_axis(smlt::Degrees(45), smlt::Vec3(1, 0, 0))
    );
}

void GameScene::create_ground() {
    // Create a textured ground plane
    auto mat = assets->new_material();
    mat->set_diffuse(smlt::Color(0.3f, 0.6f, 0.2f, 1.0f));

    ground_ = create_child<smlt::Actor>();
    auto ground_mesh = assets->new_mesh_from_procedural_cube();
    ground_->set_mesh(ground_mesh->id());
    ground_->set_material(mat->id());
    ground_->set_scale(50, 0.1f, 50);
    ground_->transform->set_position(0, -0.5f, 0);
}

void GameScene::create_player() {
    // Create the player as a simple cube (we'll replace this with a model later)
    auto mat = assets->new_material();
    mat->set_diffuse(smlt::Color(1.0f, 0.4f, 0.1f, 1.0f));

    player_ = create_child<smlt::Actor>();
    auto player_mesh = assets->new_mesh_from_procedural_cube();
    player_->set_mesh(player_mesh->id());
    player_->set_material(mat->id());
    player_->set_scale(0.8f, 0.8f, 0.8f);
    player_->transform->set_position(player_start_);
    player_->set_name("Player");
}

// --------------------------------------------------------------------------
// Input handling
// --------------------------------------------------------------------------

void GameScene::handle_input(float dt) {
    if (!player_) return;

    smlt::Vec3 movement(0, 0, 0);

    // WASD / Arrow key movement
    if (input->key_pressed(smlt::KEY_W) || input->key_pressed(smlt::KEYBOARD_CODE_UP)) {
        movement.z += 1.0f;
    }
    if (input->key_pressed(smlt::KEY_S) || input->key_pressed(smlt::KEYBOARD_CODE_DOWN)) {
        movement.z -= 1.0f;
    }
    if (input->key_pressed(smlt::KEY_A) || input->key_pressed(smlt::KEYBOARD_CODE_LEFT)) {
        movement.x -= 1.0f;
    }
    if (input->key_pressed(smlt::KEY_D) || input->key_pressed(smlt::KEYBOARD_CODE_RIGHT)) {
        movement.x += 1.0f;
    }

    // Normalize so diagonal movement isn't faster
    if (movement.length() > 0) {
        movement = movement.normalized();
        player_->transform->set_position(
            player_->position().x + movement.x * player_speed_ * dt,
            player_->position().y,
            player_->position().z + movement.z * player_speed_ * dt
        );
    }

    // Escape returns to menu
    if (input->key_just_pressed(smlt::KEY_ESCAPE)) {
        scenes->activate("menu");
    }
}
```

### Understanding the Game Scene

This is the core structure you'll use in almost every Simulant game:

1. **`on_load()`** -- create all nodes, load all assets, set up pipelines
2. **`on_activate()`** -- start runtime state (music, timers, reset positions)
3. **`on_deactivate()`** -- pause runtime state
4. **`on_update(float dt)`** -- frame-rate-dependent logic (input, camera follow)
5. **`on_fixed_update(float step)`** -- fixed-timestep logic (physics)
6. **`on_unload()`** -- destroy explicit resources like pipelines

The camera follows the player by reading the player's world position (`absolute_position()`) each frame and repositioning the camera. The `look_at()` call keeps the camera pointed at the player.

> **Key concept**: `create_child<T>()` creates a StageNode and attaches it as a child of the caller. Since we call it from the Scene, all nodes are children of the Scene's root Stage. For more see [Stage Nodes](../core-concepts/stage-nodes.md).

Build and run. You should see a green ground plane with an orange cube you can move with WASD or arrow keys. The camera follows the cube. Press Escape to return to the menu.

---

## 4. Adding Physics: Gravity, Collision, and Jumping

Our player can walk on the ground, but there's no gravity, no jumping, and no collision. Let's add Simulant's physics system using `PhysicsScene`, `DynamicBody`, and `StaticBody`.

### Update Game Scene Header

Add these members to `sources/scenes/game_scene.h`:

```cpp
private:
    // ... existing members ...

    // Physics
    smlt::PhysicsScene* physics_scene_ = nullptr;
    smlt::DynamicBody* player_body_ = nullptr;
    smlt::StaticBody* ground_body_ = nullptr;

    bool is_grounded_ = false;
    float jump_force_ = 12.0f;

    void create_physics();
```

### Physics Implementation

Add these methods to `sources/scenes/game_scene.cpp`:

```cpp
void GameScene::create_physics() {
    // Create the physics scene
    physics_scene_ = create_child<smlt::PhysicsScene>();
    physics_scene_->set_gravity(smlt::Vec3(0, -20.0f, 0));

    // Add StaticBody to ground (immovable collider)
    ground_body_ = ground_->create_mixin<smlt::StaticBody>();
    ground_body_->add_box_collider(
        ground_->aabb().dimensions(),
        smlt::PhysicsMaterial::stone()
    );

    // Add DynamicBody to player (affected by gravity)
    player_body_ = player_->create_mixin<smlt::DynamicBody>();
    player_body_->add_box_collider(
        player_->aabb().dimensions(),
        smlt::PhysicsMaterial::wood()
    );
    player_body_->set_mass(1.0f);
    player_body_->set_restitution(0.0f);  // No bounce
    player_body_->set_friction(0.5f);
    player_body_->set_linear_damping(0.0f);

    // Detect ground contact for jumping
    player_body_->signal_contact_begin().connect(
        [this](smlt::FixturePtr, smlt::FixturePtr) {
            is_grounded_ = true;
        }
    );
}
```

Now update `on_load()` to call `create_physics()`:

```cpp
void GameScene::on_load() {
    S_INFO("GameScene loading level {}", level_number_);

    create_camera();
    create_lighting();
    create_ground();
    create_player();
    create_physics();           // <-- Add this line

    pipeline_ = compositor->create_layer(this, camera_);
    pipeline_->set_clear_flags(smlt::BUFFER_CLEAR_ALL);
    pipeline_->viewport->set_color(smlt::Color(0.4f, 0.6f, 0.9f, 1.0f));
}
```

### Integrating Physics into the Update Loop

Replace the manual movement in `handle_input()` and update `on_fixed_update()`:

```cpp
void GameScene::on_fixed_update(float step) {
    Scene::on_fixed_update(step);

    if (physics_scene_) {
        physics_scene_->step_simulation(step);
    }

    // Sync visual position with physics body
    if (player_body_ && player_) {
        player_->transform->set_position(player_body_->absolute_position());
    }
}

void GameScene::handle_input(float dt) {
    if (!player_ || !player_body_) return;

    smlt::Vec3 velocity = player_body_->linear_velocity();

    // Horizontal movement
    float move_x = 0;
    float move_z = 0;

    if (input->key_pressed(smlt::KEY_W) || input->key_pressed(smlt::KEYBOARD_CODE_UP)) {
        move_z += 1.0f;
    }
    if (input->key_pressed(smlt::KEY_S) || input->key_pressed(smlt::KEYBOARD_CODE_DOWN)) {
        move_z -= 1.0f;
    }
    if (input->key_pressed(smlt::KEY_A) || input->key_pressed(smlt::KEYBOARD_CODE_LEFT)) {
        move_x -= 1.0f;
    }
    if (input->key_pressed(smlt::KEY_D) || input->key_pressed(smlt::KEYBOARD_CODE_RIGHT)) {
        move_x += 1.0f;
    }

    // Normalize and apply velocity
    if (move_x != 0 || move_z != 0) {
        float len = std::sqrt(move_x * move_x + move_z * move_z);
        move_x /= len;
        move_z /= len;

        player_body_->set_linear_velocity(
            smlt::Vec3(move_x * player_speed_, velocity.y, move_z * player_speed_)
        );
    } else {
        // Stop horizontal movement
        player_body_->set_linear_velocity(
            smlt::Vec3(0, velocity.y, 0)
        );
    }

    // Jump
    if (is_grounded_ && input->key_just_pressed(smlt::KEY_SPACE)) {
        player_body_->apply_impulse(smlt::Vec3(0, jump_force_, 0));
        is_grounded_ = false;
    }

    // Escape returns to menu
    if (input->key_just_pressed(smlt::KEY_ESCAPE)) {
        scenes->activate("menu");
    }
}
```

### How Physics Works in Simulant

Simulant's physics is built on the **Bounce** physics library, implemented entirely as Behaviours (mixins):

| Behaviour | Purpose |
|-----------|---------|
| `PhysicsScene` | The physics world -- holds gravity, runs the simulation |
| `StaticBody` | Immovable collider (ground, walls, platforms) |
| `DynamicBody` | Dynamic body affected by forces (player, crates) |
| `KinematicBody` | Script-controlled collider (moving platforms) |

Fixtures define collider shapes:
- `add_box_collider(size)` -- rectangular box
- `add_sphere_collider(radius)` -- sphere
- `add_capsule_collider(radius, height)` -- capsule (good for characters)

Collision events:
- `signal_contact_begin()` -- collision started
- `signal_contact_end()` -- collision ended

> **Further reading**: [Physics Overview](../physics/overview.md)

Build and run. The player cube now falls with gravity, lands on the ground, and you can jump with Space. Try tweaking `jump_force_` and `gravity` to get the feel you want.

---

## 5. Loading 3D Models from Prefabs

Procedural cubes work for prototyping, but a real game needs proper 3D models. Simulant uses **GLTF/GLB** as its primary model format. Models are loaded as **Prefabs** and instantiated as `PrefabInstance` nodes.

### Obtaining 3D Models

For this guide, we'll use the sample models included with Simulant. The engine ships with:
- `assets/samples/character-a.glb` -- an animated character
- `assets/samples/khronos/RiggedSimple.glb` -- a simple rigged model

You can also import your own GLB files into the `assets/meshes/` directory.

### Replacing the Player Cube with a Character Model

Update the `create_player()` method in `sources/scenes/game_scene.cpp`:

```cpp
void GameScene::create_player() {
    // Load the character prefab
    auto prefab = assets->load_prefab("assets/samples/character-a.glb");

    // Instantiate it
    auto prefab_instance = create_child<smlt::PrefabInstance>(prefab);
    prefab_instance->transform->set_position(player_start_);
    prefab_instance->set_name("Player");

    // Keep a reference to the instance for animation
    player_instance_ = prefab_instance;

    // Get the animation controller
    anim_controller_ = prefab_instance->find_mixin<smlt::AnimationController>();

    // Create the physics body attached to the instance
    player_body_ = prefab_instance->create_mixin<smlt::DynamicBody>();

    // Use a capsule collider for the character
    player_body_->add_capsule_collider(0.4f, 1.6f, smlt::PhysicsMaterial::wood());
    player_body_->set_mass(1.0f);
    player_body_->set_restitution(0.0f);
    player_body_->set_friction(0.5f);

    // Ground detection
    player_body_->signal_contact_begin().connect(
        [this](smlt::FixturePtr, smlt::FixturePtr) {
            is_grounded_ = true;
        }
    );

    // For now, disable the physics body's effect on the prefab_instance
    // by making the visual a child of the physics body
    prefab_instance->transform->set_position(player_start_);
}
```

Add the new members to `sources/scenes/game_scene.h`:

```cpp
private:
    // ... existing ...
    smlt::PrefabInstance* player_instance_ = nullptr;
    smlt::AnimationController* anim_controller_ = nullptr;
```

### Creating Level Geometry from Prefabs

You can build levels by placing GLB prefabs. Create a level file at `assets/meshes/level_1.glb` (or use any GLB file). Then load it in `on_load()`:

```cpp
void GameScene::create_level() {
    std::string level_path = "assets/meshes/level_" +
                             std::to_string(level_number_) + ".glb";

    // Try loading the level; fall back to procedural ground
    try {
        auto level_prefab = assets->load_prefab(level_path);
        auto level_instance = create_child<smlt::PrefabInstance>(level_prefab);

        // Add static colliders to level geometry
        // (This depends on your level design)
    } catch (...) {
        S_WARN("Level file not found: {}, using procedural ground", level_path);
        create_ground();
    }
}
```

> **Tip**: You can build levels in Blender, export as GLB, and place them in your assets folder. For platforms and obstacles, consider using the `Geom` node for simple box geometry if you don't have full models.

> **Further reading**: [Prefab System](../assets/prefabs.md), [Mesh Formats](../mesh_formats.md)

---

## 6. Adding Animations: Idle, Walk, Jump

Our character model comes with built-in animations from the GLB file. Let's wire them up to the player's state using the `AnimationController`.

### Animation State Machine

Add an enumeration and state tracking to `sources/scenes/game_scene.h`:

```cpp
enum class PlayerAnimState {
    IDLE,
    WALKING,
    JUMPING
};

// In the private section:
    PlayerAnimState current_anim_state_ = PlayerAnimState::IDLE;

    void update_animation();
```

### Implementing Animation Transitions

Add the `update_animation()` method to `sources/scenes/game_scene.cpp`:

```cpp
void GameScene::update_animation() {
    if (!anim_controller_) return;

    // Determine what animation we should be playing
    PlayerAnimState desired_state;

    auto velocity = player_body_ ? player_body_->linear_velocity() : smlt::Vec3::zero();
    bool is_moving = std::abs(velocity.x) > 0.1f || std::abs(velocity.z) > 0.1f;

    if (!is_grounded_) {
        desired_state = PlayerAnimState::JUMPING;
    } else if (is_moving) {
        desired_state = PlayerAnimState::WALKING;
    } else {
        desired_state = PlayerAnimState::IDLE;
    }

    // Transition if state changed
    if (desired_state != current_anim_state_) {
        current_anim_state_ = desired_state;

        // Get available animation names
        auto anims = anim_controller_->animation_names();

        switch (current_anim_state_) {
            case PlayerAnimState::IDLE:
                // Play first idle-like animation, or first available
                for (const auto& name : anims) {
                    if (name.find("idle") != std::string::npos ||
                        name.find("Idle") != std::string::npos) {
                        anim_controller_->play(name, smlt::ANIMATION_LOOP_FOREVER);
                        return;
                    }
                }
                // Fallback to first animation
                if (!anims.empty()) {
                    anim_controller_->play(anims[0], smlt::ANIMATION_LOOP_FOREVER);
                }
                break;

            case PlayerAnimState::WALKING:
                for (const auto& name : anims) {
                    if (name.find("walk") != std::string::npos ||
                        name.find("Walk") != std::string::npos ||
                        name.find("run") != std::string::npos ||
                        name.find("Run") != std::string::npos) {
                        anim_controller_->play(name, smlt::ANIMATION_LOOP_FOREVER);
                        return;
                    }
                }
                if (anims.size() > 1) {
                    anim_controller_->play(anims[1], smlt::ANIMATION_LOOP_FOREVER);
                }
                break;

            case PlayerAnimState::JUMPING:
                for (const auto& name : anims) {
                    if (name.find("jump") != std::string::npos ||
                        name.find("Jump") != std::string::npos) {
                        anim_controller_->play(name, 1);  // Play once, don't loop
                        return;
                    }
                }
                // No jump anim found -- use walk as fallback
                if (anims.size() > 1) {
                    anim_controller_->play(anims[1], 1);
                }
                break;
        }
    }
}
```

Call `update_animation()` from `on_update()`:

```cpp
void GameScene::on_update(float dt) {
    Scene::on_update(dt);
    handle_input(dt);
    update_animation();  // <-- Add this

    // Camera follows player
    if (player_instance_ && camera_) {
        auto player_pos = player_instance_->absolute_position();
        camera_->transform->set_position(
            player_pos.x,
            player_pos.y + 8.0f,
            player_pos.z - 12.0f
        );
        camera_->look_at(player_pos);
    }
}
```

### How Animation Works

The `AnimationController` is automatically created when a GLB file with animations is loaded. Key methods:

| Method | Purpose |
|--------|---------|
| `animation_names()` | Returns all animation names found in the GLB |
| `play(name, loop)` | Play an animation (`ANIMATION_LOOP_FOREVER` = loop forever) |
| `queue(name)` | Queue an animation to play after the current one finishes |
| `pause()` / `resume()` | Pause and resume playback |
| `set_animation_speed(factor)` | 1.0 = normal, 2.0 = double speed |

> **Further reading**: [Animation Overview](../animation/overview.md)

Build and run. Your character should now play different animations based on movement. If the sample character's animation names don't match the strings above, check the debug log to see what animations were found.

---

## 7. Creating a HUD: Score, Lives, Health

A HUD (Heads-Up Display) overlays gameplay information on top of the 3D world. We'll create a HUD with score, lives, and a health bar.

### HUD as a StageNode

Create `sources/systems/game_state.h`:

```cpp
#pragma once

#include <simulant/simulant.h>

struct GameState {
    int score = 0;
    int lives = 3;
    float health = 1.0f;  // 0.0 to 1.0
    int crystals_collected = 0;
    int crystals_total = 0;

    void reset() {
        score = 0;
        lives = 3;
        health = 1.0f;
        crystals_collected = 0;
        crystals_total = 0;
    }
};
```

### HUD Node

Add the HUD as a custom `StageNode` that manages UI widgets. Add to `sources/scenes/game_scene.h`:

```cpp
// Forward declaration
class HUDNode;

// In the private section of GameScene:
    HUDNode* hud_ = nullptr;
    GameState game_state_;

    void create_hud();
    void create_hud_camera();
```

Create the HUD node. Since Simulant's widget system is integrated with the Scene's StageNode tree, we create the HUD as a child of the scene and manage its widgets directly:

```cpp
// Add to sources/scenes/game_scene.h before the GameScene class:

class HUDNode : public smlt::StageNode {
public:
    smlt::ui::Label* score_label_ = nullptr;
    smlt::ui::Label* lives_label_ = nullptr;
    smlt::ui::ProgressBar* health_bar_ = nullptr;
    smlt::ui::Label* level_label_ = nullptr;

    void on_load() override {
        auto window = stage_->window();

        // Score label (top-left)
        score_label_ = stage_->create_child<smlt::ui::Label>("Score: 0");
        score_label_->set_anchor_point(0.0f, 1.0f);
        score_label_->transform->set_position_2d(
            window->coordinate_from_normalized(0.02f, 0.95f)
        );
        score_label_->set_font_size(24);

        // Lives label
        lives_label_ = stage_->create_child<smlt::ui::Label>("Lives: 3");
        lives_label_->set_anchor_point(0.0f, 1.0f);
        lives_label_->transform->set_position_2d(
            window->coordinate_from_normalized(0.02f, 0.88f)
        );
        lives_label_->set_font_size(24);

        // Health bar
        health_bar_ = stage_->create_child<smlt::ui::ProgressBar>();
        health_bar_->set_anchor_point(0.0f, 1.0f);
        health_bar_->transform->set_position_2d(
            window->coordinate_from_normalized(0.02f, 0.78f)
        );
        health_bar_->resize(200, 20);
        health_bar_->set_value(1.0f);
        health_bar_->set_text("Health");

        // Level label (top-right)
        level_label_ = stage_->create_child<smlt::ui::Label>("Level 1");
        level_label_->set_anchor_point(1.0f, 1.0f);
        level_label_->transform->set_position_2d(
            window->coordinate_from_normalized(0.98f, 0.95f)
        );
        level_label_->set_font_size(24);
        level_label_->set_text_alignment(smlt::ui::TEXT_ALIGNMENT_RIGHT);
    }

    void update_display(const GameState& state, int level) {
        score_label_->set_text("Score: " + std::to_string(state.score));
        lives_label_->set_text("Lives: " + std::to_string(state.lives));
        health_bar_->set_value(state.health);
        level_label_->set_text("Level " + std::to_string(level));

        // Color health bar based on health
        if (state.health > 0.6f) {
            health_bar_->set_foreground_color(smlt::Color::green());
        } else if (state.health > 0.3f) {
            health_bar_->set_foreground_color(smlt::Color::yellow());
        } else {
            health_bar_->set_foreground_color(smlt::Color::red());
        }
    }
};
```

Add HUD creation to the game scene. We need a separate 2D camera and render layer for the UI:

```cpp
// Add to private section of game_scene.h:
    smlt::CameraPtr hud_camera_;
    smlt::LayerPtr hud_pipeline_;

void GameScene::create_hud() {
    // Create a 2D camera for HUD rendering
    hud_camera_ = create_child<smlt::Camera2D>();
    hud_camera_->set_orthographic_projection(
        0, window->width(), 0, window->height()
    );

    // Create HUD node
    hud_ = create_child<HUDNode>();
    hud_->update_display(game_state_, level_number_);

    // Create render pipeline for HUD (foreground layer)
    hud_pipeline_ = compositor->create_layer(this, hud_camera_);
    hud_pipeline_->set_render_priority(smlt::RENDER_PRIORITY_FOREGROUND);
    hud_pipeline_->set_clear_flags(smlt::BUFFER_CLEAR_NONE);
}
```

Call `create_hud()` at the end of `on_load()`:

```cpp
void GameScene::on_load() {
    // ... existing code ...

    create_hud();  // <-- Add this
}
```

Clean up the HUD pipeline in `on_unload()`:

```cpp
void GameScene::on_unload() {
    if (hud_pipeline_) {
        hud_pipeline_->destroy();
        hud_pipeline_ = nullptr;
    }
    if (pipeline_) {
        pipeline_->destroy();
        pipeline_ = nullptr;
    }
}
```

### Collecting Crystals (Score)

Let's add crystal collectibles to make the HUD meaningful. Add crystal spawning to `GameScene`:

```cpp
// Add to game_scene.h:
    std::vector<smlt::ActorPtr> crystals_;
    void spawn_crystals();
    void check_crystal_collection();

// Add to game_scene.cpp:
void GameScene::spawn_crystals() {
    auto crystal_mat = assets->new_material();
    crystal_mat->set_diffuse(smlt::Color(0.2f, 0.8f, 1.0f, 1.0f));
    crystal_mat->set_emissive(smlt::Color(0.1f, 0.3f, 0.5f, 1.0f));

    // Place crystals in a simple pattern
    std::vector<smlt::Vec3> positions = {
        {3, 1, 3}, {-3, 1, 3}, {3, 1, -3}, {-3, 1, -3},
        {0, 2, 0}, {6, 1, 0}, {-6, 1, 0}
    };

    game_state_.crystals_total = positions.size();
    game_state_.crystals_collected = 0;

    for (const auto& pos : positions) {
        auto crystal = create_child<smlt::Actor>();
        auto mesh = assets->new_mesh_from_procedural_cube();
        crystal->set_mesh(mesh->id());
        crystal->set_material(crystal_mat->id());
        crystal->set_scale(0.4f, 0.4f, 0.4f);
        crystal->transform->set_position(pos);
        crystal->set_name("Crystal");
        crystals_.push_back(crystal);
    }
}

void GameScene::check_crystal_collection() {
    if (!player_) return;

    auto player_pos = player_->absolute_position();

    for (auto it = crystals_.begin(); it != crystals_.end(); ) {
        float dist = (*it)->absolute_position().distance_to(player_pos);
        if (dist < 1.2f) {
            // Collect this crystal
            game_state_.crystals_collected++;
            game_state_.score += 100;

            // Update HUD
            if (hud_) {
                hud_->update_display(game_state_, level_number_);
            }

            // Destroy the crystal visual
            (*it)->destroy();
            it = crystals_.erase(it);
        } else {
            ++it;
        }
    }

    // Check if all crystals collected -- level complete!
    if (crystals_.empty() && game_state_.crystals_total > 0) {
        S_INFO("Level complete! All crystals collected.");
        game_state_.score += 500;  // Bonus

        // Transition based on level number
        if (level_number_ >= 2) {
            scenes->activate("victory");
        } else {
            scenes->activate("game_level_2");
        }
    }
}
```

Call these from `on_load()` and `on_update()`:

```cpp
void GameScene::on_load() {
    // ... existing ...
    spawn_crystals();
}

void GameScene::on_update(float dt) {
    Scene::on_update(dt);
    handle_input(dt);
    update_animation();
    check_crystal_collection();  // <-- Add this

    // Rotate crystals for visual flair
    for (auto& crystal : crystals_) {
        if (crystal) {
            crystal->rotate_by(smlt::Degrees(0), smlt::Degrees(120 * dt), smlt::Degrees(60 * dt));
        }
    }

    // ... camera follow ...
}
```

> **Further reading**: [UI System Overview](../ui/overview.md), [Resource Management](../core-concepts/resource-management.md)

Build and run. You should now see a HUD with score, lives, health bar, and level number. Blue crystals float in the world -- walk into them to collect points. Collect all crystals to advance to the next level.

---

## 8. Adding Sound Effects and Music

Sound brings a game to life. We'll add background music and sound effects for collecting crystals, jumping, and taking damage.

### Loading and Playing Sounds

Simulant supports `.wav` and `.ogg` formats. Sounds are loaded through the asset manager and played via `AudioSource` nodes.

Add sound members to `sources/scenes/game_scene.h`:

```cpp
private:
    // Audio
    smlt::SoundPtr background_music_;
    smlt::SoundPtr crystal_sound_;
    smlt::SoundPtr jump_sound_;
    smlt::SoundPtr hurt_sound_;
    smlt::AudioSource* audio_source_ = nullptr;

    void load_sounds();
    void play_sfx(const smlt::SoundPtr& sound);
```

Implement sound loading and playback in `sources/scenes/game_scene.cpp`:

```cpp
void GameScene::load_sounds() {
    // Load sounds -- adjust paths to match your asset files
    // You'll need to add these .ogg or .wav files to your assets/ folder
    try {
        background_music_ = assets->load_sound("sounds/music/level_music.ogg");
    } catch (...) {
        S_WARN("Background music not found");
    }

    try {
        crystal_sound_ = assets->load_sound("sounds/sfx/crystal_collect.wav");
    } catch (...) {
        S_WARN("Crystal sound not found");
    }

    try {
        jump_sound_ = assets->load_sound("sounds/sfx/jump.wav");
    } catch (...) {
        S_WARN("Jump sound not found");
    }

    try {
        hurt_sound_ = assets->load_sound("sounds/sfx/hurt.wav");
    } catch (...) {
        S_WARN("Hurt sound not found");
    }

    // Create audio source attached to the camera for positional audio
    audio_source_ = camera_->create_child<smlt::AudioSource>();
}

void GameScene::play_sfx(const smlt::SoundPtr& sound) {
    if (audio_source_ && sound) {
        audio_source_->play_sound(sound, smlt::AUDIO_REPEAT_NONE, smlt::DISTANCE_MODEL_AMBIENT);
    }
}
```

Call `load_sounds()` from `on_load()`:

```cpp
void GameScene::on_load() {
    // ... existing ...
    load_sounds();
}
```

### Playing Music on Scene Activation

Music should start when the scene activates and stop when it deactivates:

```cpp
void GameScene::on_activate() {
    S_INFO("GameScene activated - level {}", level_number_);

    if (player_instance_) {
        player_instance_->transform->set_position(player_start_);
    }

    // Start background music
    if (audio_source_ && background_music_) {
        audio_source_->play_sound(
            background_music_,
            smlt::AUDIO_REPEAT_LOOP,
            smlt::DISTANCE_MODEL_AMBIENT
        );
    }
}

void GameScene::on_deactivate() {
    S_INFO("GameScene deactivated");

    // Stop all sounds on this source
    if (audio_source_) {
        audio_source_->stop();
    }
}
```

### Playing SFX in Response to Events

Now play sound effects when events happen:

```cpp
void GameScene::check_crystal_collection() {
    // ... existing crystal collection logic ...

    for (auto it = crystals_.begin(); it != crystals_.end(); ) {
        float dist = (*it)->absolute_position().distance_to(player_pos);
        if (dist < 1.2f) {
            game_state_.crystals_collected++;
            game_state_.score += 100;

            play_sfx(crystal_sound_);  // <-- Play collection sound

            // ... rest of collection logic ...
        }
        // ...
    }
}
```

Add jump sound to the input handler:

```cpp
    // Jump
    if (is_grounded_ && input->key_just_pressed(smlt::KEY_SPACE)) {
        player_body_->apply_impulse(smlt::Vec3(0, jump_force_, 0));
        is_grounded_ = false;
        play_sfx(jump_sound_);  // <-- Play jump sound
    }
```

### Sound Design Tips

- Use `.ogg` for music (smaller file size, supports looping)
- Use `.wav` for short sound effects (lower latency)
- `DISTANCE_MODEL_AMBIENT` disables positional attenuation -- good for music and UI sounds
- For positional SFX (explosions, footsteps), use the default distance model
- Keep sound files in `assets/sounds/sfx/` and `assets/sounds/music/` for organisation

> **Further reading**: [Audio Overview](../audio.md)

---

## 9. Multiple Levels with Scene Transitions

Let's make level 2 actually work. The key idea is that `GameScene` is parameterised by `level_number_` -- the same scene class handles all levels.

### Registering Multiple Levels

In `crystal_quest_app.cpp` we already registered two game scenes:

```cpp
scenes->register_scene<GameScene>("game", 1);       // level 1
scenes->register_scene<GameScene>("game_level_2", 2); // level 2
```

### Level-Specific Setup

Modify `on_load()` to set up level-specific content:

```cpp
void GameScene::on_load() {
    S_INFO("GameScene loading level {}", level_number_);

    create_camera();
    create_lighting();
    create_physics();
    create_player();
    create_hud();
    load_sounds();
    spawn_crystals();

    // Level-specific configuration
    switch (level_number_) {
        case 1:
            player_start_ = {0, 2, 0};
            player_speed_ = 8.0f;
            pipeline_->viewport->set_color(smlt::Color(0.4f, 0.6f, 0.9f, 1.0f));
            break;

        case 2:
            player_start_ = {0, 2, 0};
            player_speed_ = 9.0f;  // Slightly faster
            pipeline_->viewport->set_color(smlt::Color(0.2f, 0.4f, 0.6f, 1.0f));
            break;

        default:
            player_start_ = {0, 2, 0};
            break;
    }

    pipeline_ = compositor->create_layer(this, camera_);
    pipeline_->set_clear_flags(smlt::BUFFER_CLEAR_ALL);
}
```

### Adding Moving Platforms (KinematicBody)

Level 2 should be harder. Let's add a moving platform using `KinematicBody`:

```cpp
// Add to game_scene.h:
    struct MovingPlatform {
        smlt::ActorPtr visual;
        smlt::KinematicBody* body = nullptr;
        smlt::Vec3 start_pos, end_pos;
        float t = 0;
        bool forward = true;
        float speed = 1.0f;
    };
    std::vector<MovingPlatform> platforms_;
    void create_moving_platform(smlt::Vec3 start, smlt::Vec3 end, float speed);

// Add to game_scene.cpp:
void GameScene::create_moving_platform(smlt::Vec3 start, smlt::Vec3 end, float speed) {
    auto mat = assets->new_material();
    mat->set_diffuse(smlt::Color(0.6f, 0.4f, 0.8f, 1.0f));

    auto platform = create_child<smlt::Actor>();
    auto mesh = assets->new_mesh_from_procedural_cube();
    platform->set_mesh(mesh->id());
    platform->set_material(mat->id());
    platform->set_scale(3, 0.3f, 3);
    platform->transform->set_position(start);

    auto body = platform->create_mixin<smlt::KinematicBody>();
    body->add_box_collider(platform->aabb().dimensions(), smlt::PhysicsMaterial::stone());

    MovingPlatform mp;
    mp.visual = platform;
    mp.body = body;
    mp.start_pos = start;
    mp.end_pos = end;
    mp.speed = speed;
    mp.forward = true;
    mp.t = 0;
    platforms_.push_back(mp);
}
```

Update the platforms each frame:

```cpp
void GameScene::on_update(float dt) {
    Scene::on_update(dt);
    handle_input(dt);
    update_animation();
    check_crystal_collection();

    // Update moving platforms
    for (auto& platform : platforms_) {
        platform.t += platform.forward ? dt * platform.speed : -dt * platform.speed;
        if (platform.t >= 1.0f) { platform.t = 1.0f; platform.forward = false; }
        if (platform.t <= 0.0f) { platform.t = 0.0f; platform.forward = true; }

        smlt::Vec3 pos = smlt::lerp(platform.start_pos, platform.end_pos, platform.t);
        platform.body->move_to(pos);
    }

    // Rotate crystals
    for (auto& crystal : crystals_) {
        if (crystal) {
            crystal->rotate_by(smlt::Degrees(0), smlt::Degrees(120 * dt), smlt::Degrees(60 * dt));
        }
    }

    // Camera follow
    if (player_instance_ && camera_) {
        auto player_pos = player_instance_->absolute_position();
        camera_->transform->set_position(
            player_pos.x, player_pos.y + 8.0f, player_pos.z - 12.0f
        );
        camera_->look_at(player_pos);
    }
}
```

Spawn platforms only in level 2:

```cpp
void GameScene::spawn_crystals() {
    // ... existing crystal logic ...

    // Add moving platforms in level 2
    if (level_number_ >= 2) {
        create_moving_platform({5, 1, 0}, {-5, 1, 0}, 0.5f);
        create_moving_platform({0, 3, 5}, {0, 3, -5}, 0.8f);
    }
}
```

### Scene Transition Flow

The transition flow for our game is:

```
Menu -> GameScene (level 1) -> GameScene (level 2) -> Victory
  |                              |
  +-> Settings                   +-> GameOver (if lives = 0)
```

When all crystals in level 1 are collected, we activate `"game_level_2"`. When all crystals in level 2 are collected, we activate `"victory"`.

> **Further reading**: [Scene Management](../core-concepts/scene-management.md)

---

## 10. Game Over and Victory Conditions

### Game Over Scene

The Game Over scene shows when the player runs out of lives. Create `sources/scenes/game_over_scene.h`:

```cpp
#pragma once

#include <simulant/simulant.h>

class GameOverScene : public smlt::Scene {
public:
    GameOverScene(smlt::Window* window)
        : Scene(window) {}

    void on_load() override;
    void on_unload() override;

private:
    smlt::CameraPtr ui_camera_;
    smlt::LayerPtr pipeline_;
};
```

Create `sources/scenes/game_over_scene.cpp`:

```cpp
#include "game_over_scene.h"

void GameOverScene::on_load() {
    S_INFO("GameOverScene loading");

    ui_camera_ = create_child<smlt::Camera2D>();
    ui_camera_->set_orthographic_projection(
        0, window->width(), 0, window->height()
    );

    // Title
    auto title_coord = window->coordinate_from_normalized(0.5f, 0.6f);
    auto title = create_child<smlt::ui::Label>("Game Over");
    title->set_anchor_point(0.5f, 0.5f);
    title->transform->set_position_2d(smlt::Vec2(title_coord.x, title_coord.y));
    title->set_text_alignment(smlt::ui::TEXT_ALIGNMENT_CENTER);
    title->set_font_size(48);
    title->set_text_color(smlt::Color::red());

    // Retry button
    auto btn_coord = window->coordinate_from_normalized(0.5f, 0.4f);
    auto retry_btn = create_child<smlt::ui::Button>("Try Again");
    retry_btn->set_anchor_point(0.5f, 0.5f);
    retry_btn->transform->set_position_2d(smlt::Vec2(btn_coord.x, btn_coord.y));
    retry_btn->resize(200, -1);
    retry_btn->set_text_alignment(smlt::ui::TEXT_ALIGNMENT_CENTER);
    retry_btn->signal_activated().connect([this]() {
        scenes->activate("game");
    });

    // Menu button
    auto menu_coord = window->coordinate_from_normalized(0.5f, 0.3f);
    auto menu_btn = create_child<smlt::ui::Button>("Main Menu");
    menu_btn->set_anchor_point(0.5f, 0.5f);
    menu_btn->transform->set_position_2d(smlt::Vec2(menu_coord.x, menu_coord.y));
    menu_btn->resize(200, -1);
    menu_btn->set_text_alignment(smlt::ui::TEXT_ALIGNMENT_CENTER);
    menu_btn->signal_activated().connect([this]() {
        scenes->activate("menu");
    });

    pipeline_ = compositor->create_layer(this, ui_camera_);
    pipeline_->set_clear_flags(smlt::BUFFER_CLEAR_ALL);
    pipeline_->viewport->set_color(smlt::Color(0.1f, 0.05f, 0.05f, 1.0f));
}

void GameOverScene::on_unload() {
    if (pipeline_) {
        pipeline_->destroy();
        pipeline_ = nullptr;
    }
}
```

### Victory Scene

The Victory scene shows when the player completes all levels. Create `sources/scenes/victory_scene.h`:

```cpp
#pragma once

#include <simulant/simulant.h>

class VictoryScene : public smlt::Scene {
public:
    VictoryScene(smlt::Window* window)
        : Scene(window) {}

    void on_load() override;
    void on_activate() override;
    void on_unload() override;

private:
    smlt::CameraPtr ui_camera_;
    smlt::LayerPtr pipeline_;
    smlt::ui::Label* score_label_ = nullptr;
    smlt::SoundPtr victory_sound_;
    smlt::AudioSource* audio_source_ = nullptr;
};
```

Create `sources/scenes/victory_scene.cpp`:

```cpp
#include "victory_scene.h"

void VictoryScene::on_load() {
    S_INFO("VictoryScene loading");

    ui_camera_ = create_child<smlt::Camera2D>();
    ui_camera_->set_orthographic_projection(
        0, window->width(), 0, window->height()
    );

    // Title
    auto title_coord = window->coordinate_from_normalized(0.5f, 0.65f);
    auto title = create_child<smlt::ui::Label>("You Win!");
    title->set_anchor_point(0.5f, 0.5f);
    title->transform->set_position_2d(smlt::Vec2(title_coord.x, title_coord.y));
    title->set_text_alignment(smlt::ui::TEXT_ALIGNMENT_CENTER);
    title->set_font_size(56);
    title->set_text_color(smlt::Color::yellow());

    // Final score
    auto score_coord = window->coordinate_from_normalized(0.5f, 0.52f);
    score_label_ = create_child<smlt::ui::Label>("Score: 0");
    score_label_->set_anchor_point(0.5f, 0.5f);
    score_label_->transform->set_position_2d(smlt::Vec2(score_coord.x, score_coord.y));
    score_label_->set_text_alignment(smlt::ui::TEXT_ALIGNMENT_CENTER);
    score_label_->set_font_size(32);

    // Play Again button
    auto play_coord = window->coordinate_from_normalized(0.5f, 0.38f);
    auto play_btn = create_child<smlt::ui::Button>("Play Again");
    play_btn->set_anchor_point(0.5f, 0.5f);
    play_btn->transform->set_position_2d(smlt::Vec2(play_coord.x, play_coord.y));
    play_btn->resize(200, -1);
    play_btn->set_text_alignment(smlt::ui::TEXT_ALIGNMENT_CENTER);
    play_btn->signal_activated().connect([this]() {
        scenes->activate("menu");
    });

    // Quit button
    auto quit_coord = window->coordinate_from_normalized(0.5f, 0.28f);
    auto quit_btn = create_child<smlt::ui::Button>("Quit");
    quit_btn->set_anchor_point(0.5f, 0.5f);
    quit_btn->transform->set_position_2d(smlt::Vec2(quit_coord.x, quit_coord.y));
    quit_btn->resize(200, -1);
    quit_btn->set_text_alignment(smlt::ui::TEXT_ALIGNMENT_CENTER);
    quit_btn->signal_activated().connect([this]() {
        app->shutdown();
    });

    pipeline_ = compositor->create_layer(this, ui_camera_);
    pipeline_->set_clear_flags(smlt::BUFFER_CLEAR_ALL);
    pipeline_->viewport->set_color(smlt::Color(0.05f, 0.1f, 0.05f, 1.0f));

    // Try loading victory fanfare
    try {
        victory_sound_ = assets->load_sound("sounds/music/victory.ogg");
    } catch (...) {
        S_WARN("Victory sound not found");
    }
    audio_source_ = create_child<smlt::AudioSource>();
}

void VictoryScene::on_activate() {
    S_INFO("VictoryScene activated");

    // Display final score from shared game state
    // (In a real game you'd pass this through a service or global state)
    if (score_label_) {
        score_label_->set_text("Congratulations!");
    }

    // Play victory music
    if (audio_source_ && victory_sound_) {
        audio_source_->play_sound(
            victory_sound_, smlt::AUDIO_REPEAT_LOOP, smlt::DISTANCE_MODEL_AMBIENT
        );
    }
}

void VictoryScene::on_unload() {
    if (pipeline_) {
        pipeline_->destroy();
        pipeline_ = nullptr;
    }
}
```

### Tying Health and Lives to the Game

Add health and lives management to `GameScene`. In `handle_input()`, add hazard zones (areas that damage the player):

```cpp
void GameScene::on_update(float dt) {
    // ... existing ...

    // Check if player fell off the world
    if (player_instance_) {
        auto pos = player_instance_->absolute_position();
        if (pos.y < -10) {
            take_damage(1.0f);  // Instant death
        }
    }
}

void GameScene::take_damage(float amount) {
    game_state_.health -= amount;
    play_sfx(hurt_sound_);

    if (hud_) {
        hud_->update_display(game_state_, level_number_);
    }

    if (game_state_.health <= 0) {
        game_state_.lives--;

        if (game_state_.lives <= 0) {
            // Game Over
            scenes->activate("game_over");
        } else {
            // Respawn
            game_state_.health = 1.0f;
            if (player_body_) {
                player_body_->move_to(player_start_);
                player_body_->set_linear_velocity(smlt::Vec3::zero());
            }
            if (hud_) {
                hud_->update_display(game_state_, level_number_);
            }
        }
    }
}
```

---

## 11. Settings Scene: Volume Control

The Settings scene lets players adjust music and SFX volume. Create `sources/scenes/settings_scene.h`:

```cpp
#pragma once

#include <simulant/simulant.h>

class SettingsScene : public smlt::Scene {
public:
    SettingsScene(smlt::Window* window)
        : Scene(window) {}

    void on_load() override;
    void on_unload() override;

private:
    smlt::CameraPtr ui_camera_;
    smlt::LayerPtr pipeline_;

    smlt::ui::ProgressBar* music_volume_bar_ = nullptr;
    smlt::ui::ProgressBar* sfx_volume_bar_ = nullptr;
    smlt::ui::Label* music_label_ = nullptr;
    smlt::ui::Label* sfx_label_ = nullptr;

    float music_volume_ = 1.0f;
    float sfx_volume_ = 1.0f;

    void create_ui();
    void update_volume_labels();
    void apply_volumes();
};
```

Create `sources/scenes/settings_scene.cpp`:

```cpp
#include "settings_scene.h"

void SettingsScene::on_load() {
    S_INFO("SettingsScene loading");

    ui_camera_ = create_child<smlt::Camera2D>();
    ui_camera_->set_orthographic_projection(
        0, window->width(), 0, window->height()
    );

    create_ui();

    pipeline_ = compositor->create_layer(this, ui_camera_);
    pipeline_->set_clear_flags(smlt::BUFFER_CLEAR_ALL);
    pipeline_->viewport->set_color(smlt::Color(0.1f, 0.15f, 0.2f, 1.0f));
}

void SettingsScene::on_unload() {
    if (pipeline_) {
        pipeline_->destroy();
        pipeline_ = nullptr;
    }
}

void SettingsScene::create_ui() {
    auto title_coord = window->coordinate_from_normalized(0.5f, 0.85f);
    auto title = create_child<smlt::ui::Label>("Settings");
    title->set_anchor_point(0.5f, 0.5f);
    title->transform->set_position_2d(smlt::Vec2(title_coord.x, title_coord.y));
    title->set_text_alignment(smlt::ui::TEXT_ALIGNMENT_CENTER);
    title->set_font_size(36);

    int label_x = window->coordinate_from_normalized(0.25f, 0).x;
    int bar_x = window->coordinate_from_normalized(0.35f, 0).x;
    int y = window->coordinate_from_normalized(0, 0.65f).y;
    int spacing = 60;

    // Music volume
    music_label_ = create_child<smlt::ui::Label>("Music Volume: 100%");
    music_label_->set_anchor_point(0.0f, 0.5f);
    music_label_->transform->set_position_2d(smlt::Vec2(label_x, y));
    music_label_->set_font_size(20);

    music_volume_bar_ = create_child<smlt::ui::ProgressBar>();
    music_volume_bar_->set_anchor_point(0.0f, 0.5f);
    music_volume_bar_->transform->set_position_2d(smlt::Vec2(bar_x, y));
    music_volume_bar_->resize(300, 20);
    music_volume_bar_->set_value(music_volume_);
    music_volume_bar_->set_text("Music");
    music_volume_bar_->signal_pointer_down().connect([this](int, smlt::Vec2 pos) {
        // Simple click-to-set volume
        float rel_x = (float)(pos.x - bar_x) / 300.0f;
        music_volume_ = std::clamp(rel_x, 0.0f, 1.0f);
        music_volume_bar_->set_value(music_volume_);
        update_volume_labels();
        apply_volumes();
    });

    y -= spacing;

    // SFX volume
    sfx_label_ = create_child<smlt::ui::Label>("SFX Volume: 100%");
    sfx_label_->set_anchor_point(0.0f, 0.5f);
    sfx_label_->transform->set_position_2d(smlt::Vec2(label_x, y));
    sfx_label_->set_font_size(20);

    sfx_volume_bar_ = create_child<smlt::ui::ProgressBar>();
    sfx_volume_bar_->set_anchor_point(0.0f, 0.5f);
    sfx_volume_bar_->transform->set_position_2d(smlt::Vec2(bar_x, y));
    sfx_volume_bar_->resize(300, 20);
    sfx_volume_bar_->set_value(sfx_volume_);
    sfx_volume_bar_->set_text("SFX");
    sfx_volume_bar_->signal_pointer_down().connect([this](int, smlt::Vec2 pos) {
        float rel_x = (float)(pos.x - bar_x) / 300.0f;
        sfx_volume_ = std::clamp(rel_x, 0.0f, 1.0f);
        sfx_volume_bar_->set_value(sfx_volume_);
        update_volume_labels();
        apply_volumes();
    });

    y -= spacing + 20;

    // Back button
    auto back_coord = window->coordinate_from_normalized(0.5f, 0.25f);
    auto back_btn = create_child<smlt::ui::Button>("Back");
    back_btn->set_anchor_point(0.5f, 0.5f);
    back_btn->transform->set_position_2d(smlt::Vec2(back_coord.x, back_coord.y));
    back_btn->resize(200, -1);
    back_btn->set_text_alignment(smlt::ui::TEXT_ALIGNMENT_CENTER);
    back_btn->signal_activated().connect([this]() {
        scenes->activate("menu");
    });

    update_volume_labels();
}

void SettingsScene::update_volume_labels() {
    if (music_label_) {
        music_label_->set_text("Music Volume: " +
            std::to_string((int)(music_volume_ * 100)) + "%");
    }
    if (sfx_label_) {
        sfx_label_->set_text("SFX Volume: " +
            std::to_string((int)(sfx_volume_ * 100)) + "%");
    }
}

void SettingsScene::apply_volumes() {
    // Adjust the master volume of the sound driver
    // In Simulant, volume is controlled per AudioSource via set_gain()
    // For a real game, you'd store these values and apply them
    // to all active audio sources
    app->sound_driver->set_master_volume(music_volume_);
}
```

> **Note**: `SoundDriver::set_master_volume()` controls the master output. In a full implementation, you would track separate music and SFX audio sources and apply volumes independently. For simplicity, we use master volume here.

---

## 12. Polish and Debugging

With the core game complete, let's add polish and learn the debugging tools.

### Debug Drawing

Enable Simulant's debug visualiser to see physics colliders, bounding boxes, and frame timing. In `on_load()`:

```cpp
void GameScene::on_load() {
    // ... existing ...

    // Enable debug drawing (toggle with F1)
    debug_ = create_child<smlt::Debug>();
}
```

Add to `game_scene.h`:

```cpp
    smlt::Debug* debug_ = nullptr;
```

Toggle debug mode with a key:

```cpp
void GameScene::handle_input(float dt) {
    // ... existing ...

    // Toggle debug overlay with F1
    if (input->key_just_pressed(smlt::KEY_F1)) {
        if (debug_) {
            debug_->set_visible(!debug_->is_visible());
        }
    }
}
```

### Adding a Stats Panel

The `StatsPanel` node displays FPS, draw calls, and memory usage:

```cpp
void GameScene::on_load() {
    // ... existing ...

    // FPS counter
    auto stats = create_child<smlt::StatsPanel>();
    stats->set_anchor_point(1.0f, 1.0f);
    stats->transform->set_position_2d(
        window->coordinate_from_normalized(0.99f, 0.99f)
    );
}
```

### Environmental Polish

Add atmosphere with a skybox and ambient particles:

```cpp
void GameScene::create_lighting() {
    lighting->set_ambient_light(smlt::Color(0.4f, 0.4f, 0.5f, 1.0f));

    auto sun = create_child<smlt::DirectionalLight>();
    sun->set_color(smlt::Color(1.0f, 0.95f, 0.8f, 1.0f));
    sun->set_intensity(1.0f);
    sun->transform->set_rotation(
        smlt::Quaternion::angle_axis(smlt::Degrees(45), smlt::Vec3(1, 0, 0))
    );

    // Add a skybox (desktop only -- images may be too large for Dreamcast)
    if (get_platform()->name() != "dreamcast") {
        create_child<smlt::Skybox>("assets/samples/skyboxes/TropicalSunnyDay");
    }
}
```

### Coroutines for Timed Events

Use coroutines for delayed actions. For example, show a "Level Start" message:

```cpp
void GameScene::on_activate() {
    // ... existing ...

    // Show level label briefly using a coroutine
    cr_async([this]() {
        // Show "Level 1" for 2 seconds
        if (hud_ && hud_->level_label_) {
            hud_->level_label_->set_text("Level " + std::to_string(level_number_));
        }
        cr_yield_for(smlt::Seconds(2.0f));
        // Reset label
        if (hud_ && hud_->level_label_) {
            hud_->level_label_->set_text("");
        }
    });
}
```

> **Further reading**: [Coroutines](../coroutines.md), [Debug Drawing](../utilities/debug-drawing.md), [Profiling](../utilities/profiling.md)

### Common Debugging Tips

| Problem | Debug Step |
|---------|-----------|
| Object not visible | Check `is_part_of_active_pipeline()`, verify camera position |
| Object not moving | Check physics is enabled, verify `step_simulation()` is called |
| Collision not working | Ensure both bodies have fixtures, check fixture sizes |
| Sound not playing | Verify file path, check `audio_source_` is not null |
| UI not showing | Check UI camera, verify layer has `RENDER_PRIORITY_FOREGROUND` |
| Crash on scene switch | Ensure pipelines are destroyed in `on_unload()` |

---

## 13. Building and Packaging for Distribution

Your game works locally -- now let's package it for others to play.

### Building for Desktop

```bash
cd ~/Projects/crystal-quest
simulant build
```

This creates a binary in `build/linux/crystal-quest`. Run it with:

```bash
simulant run
```

### Building for Other Platforms

If you have Docker installed, you can cross-compile:

```bash
# Windows build
simulant build windows

# Dreamcast build
simulant build dreamcast
```

### Packaging

Create distributable packages:

```bash
# Package for all target platforms
simulant package

# Package for a specific platform
simulant package linux
```

Packages are generated in the `packages/` directory.

### Asset Embedding

On desktop platforms, assets are loaded from disk at runtime. For Dreamcast and other constrained platforms, Simulant embeds assets directly into the executable during the build process. This is handled automatically.

To ensure all assets are included, verify your `simulant.json` has the correct `asset_paths`:

```json
{
    "name": "crystal-quest",
    "version": "1.0.0",
    "description": "A 3D crystal collecting platformer",
    "author": "Your Name",
    "target_platforms": ["linux"],
    "asset_paths": ["assets"],
    "core_assets": true
}
```

### Distributing Your Game

For Linux distribution, you typically share:
1. The executable from `build/linux/`
2. The entire `assets/` directory alongside it

Create a simple launch script or package everything into a tarball:

```bash
tar czf crystal-quest-v1.0.0.tar.gz \
    build/linux/crystal-quest \
    assets/
```

For Dreamcast, the `.bin` file generated by `simulant package dreamcast` can be burned to a CD-R or run in an emulator.

---

## Summary: What You Built

You've built a complete 3D platformer game with:

| Feature | Simulant System |
|---------|----------------|
| Main menu with buttons | UI System (Labels, Buttons, Frames) |
| 3D game world with camera | Scene, Camera3D, Layers |
| Player character | PrefabInstance, AnimationController |
| Physics (gravity, collision, jumping) | PhysicsScene, DynamicBody, StaticBody |
| Collectible items | Actors with procedural meshes |
| HUD (score, lives, health) | UI widgets on a separate 2D camera layer |
| Sound effects and music | AudioSource, SoundDriver |
| Multiple levels | Parameterised Scene with SceneManager |
| Game over / victory screens | Dedicated Scene classes |
| Settings (volume control) | UI with interactive ProgressBar |
| Debug tools | Debug node, StatsPanel |

## Project File Reference

Here's the complete file structure you should have:

```
crystal-quest/
|-- sources/
|   |-- main.cpp                     # Entry point
|   |-- crystal_quest_app.h          # Application header
|   |-- crystal_quest_app.cpp        # Application init, scene registration
|   |-- scenes/
|   |   |-- menu_scene.h             # Main menu
|   |   |-- menu_scene.cpp
|   |   |-- game_scene.h             # Gameplay (parameterised by level)
|   |   |-- game_scene.cpp
|   |   |-- settings_scene.h         # Volume settings
|   |   |-- settings_scene.cpp
|   |   |-- game_over_scene.h        # Game over screen
|   |   |-- game_over_scene.cpp
|   |   |-- victory_scene.h          # Victory screen
|   |   |-- victory_scene.cpp
|   |-- systems/
|       |-- game_state.h             # Shared game state struct
|-- assets/
|   |-- sounds/
|   |   |-- music/
|   |   |   |-- level_music.ogg      # Background music
|   |   |   |-- victory.ogg          # Victory fanfare
|   |   |-- sfx/
|   |       |-- crystal_collect.wav  # Crystal pickup
|   |       |-- jump.wav             # Jump sound
|   |       |-- hurt.wav             # Damage sound
|   |-- meshes/
|       |-- level_1.glb              # Level 1 geometry (optional)
|       |-- level_2.glb              # Level 2 geometry (optional)
```

## Where to Go Next

- **[3D Game Development Guide](3d-games.md)** -- Advanced 3D techniques
- **[2D Game Development Guide](2d-games.md)** -- Building 2D games
- **[Performance Optimization](performance.md)** -- Making your game run faster
- **[Asset Pipeline](asset-pipeline.md)** -- Importing and preparing assets
- **[Packaging & Distribution](packaging.md)** -- Shipping your game
- **[Particle Systems](../rendering/particle-systems.md)** -- Adding visual effects
- **[Mesh Instancer](../rendering/mesh-instancer.md)** -- Efficient instanced rendering

## API Quick Reference

| Task | Class / Method |
|------|---------------|
| Create a scene | `class MyScene : public smlt::Scene` |
| Register a scene | `scenes->register_scene<MyScene>("name")` |
| Switch scenes | `scenes->activate("name")` |
| Create an actor | `create_child<smlt::Actor>()` |
| Load a prefab | `assets->load_prefab("path.glb")` |
| Play animation | `anim_controller->play("name", ANIMATION_LOOP_FOREVER)` |
| Create physics | `create_child<PhysicsScene>()` |
| Create a collider | `body->add_box_collider(size)` |
| Create UI label | `create_child<smlt::ui::Label>("text")` |
| Create UI button | `create_child<smlt::ui::Button>("text")` |
| Play sound | `audio_source->play_sound(sound, repeat, distance_model)` |
| Create camera | `create_child<smlt::Camera3D>()` |
| Create render layer | `compositor->create_layer(stage, camera)` |
| Start coroutine | `cr_async([&]() { cr_yield_for(Seconds(2)); })` |

---

*Happy game development!*
