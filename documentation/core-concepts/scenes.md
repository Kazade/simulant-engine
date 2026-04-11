# Scenes

A Scene represents a single screen or state in your game. Think of each Scene as a self-contained unit of gameplay or UI: a main menu, a loading screen, a level, a game-over screen, and so on. The Scene system gives you a clean way to organize and switch between these different states without entangling their logic.

This document covers everything you need to know about creating, managing, and organizing Scenes in Simulant.

**Related documentation:**
- [Scene Management](scene-management.md) -- Scene transitions, preloading, and advanced activation behaviours
- [Stage Nodes](stage-nodes.md) -- The StageNode hierarchy that lives inside a Scene
- [Application](application.md) -- How Scenes are registered and launched from the Application

---

## What Is a Scene and Why Does It Exist?

A `Scene` is a container that holds a tree of [StageNodes](stage-nodes.md) (cameras, actors, lights, UI elements, etc.) and provides lifecycle hooks so you can control what happens when the Scene loads, activates, updates, and unloads.

The Scene system exists to solve several common problems in game development:

1. **Separation of concerns** -- Each screen or game state gets its own class. The menu code never mixes with the gameplay code.
2. **Resource management** -- Each Scene owns its own `AssetManager`. When the Scene unloads, its assets can be released automatically, keeping memory usage low.
3. **Lifecycle control** -- Well-defined hooks (`on_load`, `on_activate`, `on_update`, etc.) tell you exactly when to create objects, start music, or save state.
4. **Smooth transitions** -- The SceneManager can preload a Scene in the background while the current one is still running, enabling loading screens and seamless level transitions.

Visually, you can think of the architecture like this:

```
Application
  |-- SceneManager
        |-- "menu"     -> MenuScene (currently active)
        |-- "game"     -> GameScene (preloaded)
        |-- "settings" -> SettingsScene (not yet loaded)
```

Each Scene contains its own stage-node tree, its own asset manager, and its own compositor for setting up render pipelines.

---

## Creating a Scene

Creating a Scene involves two steps: **defining** your Scene class, and **registering** it with the `SceneManager`.

### Step 1: Define Your Scene Class

Every Scene is a subclass of `smlt::Scene`. You must provide a constructor that accepts a `Window*` and forwards it to the parent `Scene` constructor. At minimum, you must override the pure-virtual `on_load()` method:

```cpp
#include <simulant/simulant.h>

using namespace smlt;

class MyScene : public Scene {
public:
    MyScene(Window* window)
        : Scene(window) {}

private:
    // on_load is pure virtual -- you MUST implement it
    void on_load() override {
        // Set up cameras, lights, UI, actors, etc.
        S_INFO("MyScene is loading");
    }
};
```

### Step 2: Register the Scene

Scenes are registered with the `SceneManager` (accessible via the `scenes` property from anywhere in your Application or Scene). Registration is typically done in your Application's `init()` method:

```cpp
class MyGame : public Application {
public:
    MyGame(const AppConfig& config)
        : Application(config) {}

    bool init() override {
        // Register scenes with string identifiers
        scenes->register_scene<MyScene>("main");
        scenes->register_scene<MenuScene>("menu");
        scenes->register_scene<GameScene>("game");

        return true;
    }
};
```

The name `"main"` is special: it is the first Scene the engine attempts to activate when the application starts. Make sure you register a Scene with this name.

### Passing Constructor Arguments

You can pass additional arguments to your Scene's constructor at registration time:

```cpp
class GameScene : public Scene {
public:
    GameScene(Window* window, int level_number)
        : Scene(window), level_number_(level_number) {}

private:
    int level_number_;

    void on_load() override {
        S_INFO("Loading level {}", level_number_);
    }
};

// In Application::init():
scenes->register_scene<GameScene>("game", 5);  // level_number = 5
```

---

## Scene Lifecycle Methods

A Scene goes through a well-defined lifecycle. Understanding the order and purpose of each method is essential for writing correct game logic.

### Lifecycle Overview

```
Registration  -->  Loading  -->  Activation  -->  Updating  -->  Deactivation  -->  Unloading
```

Here is every lifecycle method you can override, in the order they are called:

| Method | When Called | Purpose |
|--------|-------------|---------|
| `on_pre_load()` | Before `on_load()` | Optional hook for pre-loading setup |
| `on_load()` | When the Scene is first loaded | Create StageNodes, load assets, set up pipelines |
| `on_post_unload()` | After `on_unload()` | Optional cleanup after unload |
| `on_unload()` | When the Scene is unloaded | Release resources, destroy nodes |
| `on_activate()` | When the Scene becomes the active Scene | Start music, reset timers, show UI |
| `on_deactivate()` | When the Scene stops being active | Pause music, save transient state |
| `on_update(float dt)` | Every frame, while active | Frame-rate-dependent updates (animation, input) |
| `on_fixed_update(float step)` | At a fixed timestep (60 Hz), while active | Physics updates, deterministic simulation |
| `on_late_update(float dt)` | After all `on_update` calls, while active | Camera follow, post-processing updates |

### Detailed Method Reference

#### `on_pre_load()`

Called immediately before `on_load()`. Override this if you need to do setup work before the main loading logic runs. Default implementation does nothing.

```cpp
void on_pre_load() override {
    S_INFO("About to load the scene");
}
```

#### `on_load()`

**This is the only pure-virtual method.** You must implement it. Called once when the Scene is first loaded (either via `activate()`, `preload()`, or `preload_in_background()`). This is where you:

- Create cameras, lights, actors, and UI elements
- Load textures, meshes, sounds, and fonts via `assets->`
- Set up render pipelines (Layers) via `compositor->`

```cpp
void on_load() override {
    // Create a camera
    camera_ = create_node<Camera3D>();
    camera_->transform->set_position(0, 5, -10);
    camera_->look_at(0, 0, 0);

    // Add a light
    auto light = create_node<DirectionalLight>();
    light->set_direction(1, -1, 0);

    // Set up a render pipeline
    pipeline_ = compositor->create_layer(this, camera_);

    // Load assets
    auto texture = assets->load_texture("hero.png");
    auto mesh = assets->load_mesh("hero.glb");
}
```

#### `on_unload()`

Called when the Scene is unloaded. Use this to destroy pipelines, release scene-specific resources, and perform any explicit cleanup. The engine will automatically destroy StageNodes that belong to the Scene, but you may need to manually destroy things like Layers (render pipelines):

```cpp
void on_unload() override {
    // Destroy the render pipeline
    if (pipeline_) {
        pipeline_->destroy();
    }
}
```

#### `on_post_unload()`

Called after `on_unload()`. Use this for any final cleanup that needs to happen after the unload process completes. Default implementation does nothing.

#### `on_activate()`

Called when the Scene becomes the active Scene. A Scene is loaded once but may be activated and deactivated multiple times. Use this to:

- Start or resume background music
- Reset gameplay timers
- Show/hide UI elements
- Initialize runtime state (not asset loading)

```cpp
void on_activate() override {
    start_time_ = app->time_keeper->now_in_us();
    source_->play_sound(background_music_, AUDIO_REPEAT_LOOP, DISTANCE_MODEL_AMBIENT);
}
```

#### `on_deactivate()`

Called when the Scene stops being the active Scene. Use this to:

- Pause or stop music
- Save transient state
- Hide UI elements

```cpp
void on_deactivate() override {
    // Stop the background music
    if (source_) {
        source_->stop();
    }
}
```

#### `on_update(float dt)`

Called every frame while the Scene is active. `dt` is the time in seconds since the last frame. Use this for frame-rate-dependent logic:

- Player input and movement
- Animation updates
- AI updates
- Particle system control

```cpp
void on_update(float dt) override {
    // Always call the parent implementation so child nodes get updated
    Scene::on_update(dt);

    // Update gameplay logic
    player_->move(input->axis_value("horizontal") * speed_ * dt);
}
```

#### `on_fixed_update(float step)`

Called at a fixed timestep (typically 60 Hz), independent of the frame rate. Use this for physics and deterministic simulation:

- Rigid body physics
- Collision detection
- Network tick updates

```cpp
void on_fixed_update(float step) override {
    Scene::on_fixed_update(step);

    // Apply physics step
    simulation_->step(step);
}
```

#### `on_late_update(float dt)`

Called after all `on_update()` calls have completed. Use this for work that depends on the results of other updates:

- Camera follow logic (so the camera reacts to the player's new position)
- Post-processing adjustments
- UI anchoring to world objects

```cpp
void on_late_update(float dt) override {
    Scene::on_late_update(dt);

    // Camera follows player after player has moved
    camera_->transform->set_position(player_->position() + offset_);
}
```

---

## Scene Properties

Every Scene has access to several important properties that give you access to the broader engine. These are exposed as member variables you can access directly with `this->` or simply by name:

| Property | Type | Description |
|----------|------|-------------|
| `window` | `Window*` | The window the Scene belongs to. Access resolution, input, and compositor. |
| `app` | `Application*` | The Application instance. Access time, sound driver, and global config. |
| `input` | `InputManager*` | Shortcut to `window->input`. Read axis values and button states. |
| `scenes` | `SceneManager*` | The SceneManager. Activate other scenes, check load state. |
| `compositor` | `SceneCompositor*` | Create and manage render pipelines (Layers). |
| `lighting` | `LightingSettings` | Configure ambient light for the Scene. |
| `assets` | `AssetManager*` | Scene-local asset manager. Loaded assets are tied to this Scene's lifecycle. |

### Using Properties

```cpp
class GameScene : public Scene {
public:
    GameScene(Window* window)
        : Scene(window) {}

    void on_load() override {
        // Access the window dimensions
        S_INFO("Window size: {}x{}", window->width(), window->height());

        // Load a texture using the scene-local asset manager
        auto texture = assets->load_texture("background.png");

        // Access the global sound driver (persists across scenes)
        auto music = app->sound_driver;

        // Activate another scene from within this scene
        // scenes->activate("menu");
    }

    void on_update(float dt) override {
        Scene::on_update(dt);

        // Read input
        float move = input->axis_value("horizontal");

        // Get time information
        float fps = app->time_keeper->fps();
    }

    void on_activate() override {
        // Set the ambient lighting
        lighting->set_ambient_light(Color(0.2f, 0.2f, 0.3f, 1.0f));
    }
};
```

### Scene-Local vs. Shared Assets

The `assets` property gives you a Scene-local `AssetManager`. Assets loaded through it are released when the Scene unloads, which helps manage memory efficiently:

```cpp
void on_load() override {
    // Scene-local asset -- released when this scene unloads
    auto character_mesh = assets->load_mesh("character.glb");

    // Shared asset -- persists globally, never released
    auto shared_tex = window->shared_assets->load_texture("ui_common.png");
}
```

Use `assets->` for level-specific resources and `window->shared_assets->` for resources shared across the entire game (UI elements, fonts, common sounds).

---

## Creating StageNodes Within a Scene

Because `Scene` inherits from `StageNodeManager`, you can create StageNodes directly inside a Scene using `create_node<T>()`:

```cpp
void on_load() override {
    // Create a 3D camera
    auto camera = create_node<Camera3D>();
    camera->transform->set_position(0, 10, -15);

    // Create a directional light
    auto light = create_node<DirectionalLight>();
    light->set_color(Color(1.0f, 0.95f, 0.8f, 1.0f));

    // Create a Stage for grouping objects
    auto environment = create_node<Stage>();

    // Create an Actor with a mesh
    auto mesh = assets->load_mesh("tree.glb");
    auto tree = create_node<Actor>(mesh);
    tree->transform->set_position(5, 0, 0);

    // Create UI elements
    auto label = create_node<ui::Label>("Hello, Simulant!");
    label->set_anchor_point(0.5f, 0.5f);

    // Create a particle system
    auto fire_script = assets->load_particle_script(
        ParticleScript::BuiltIns::FIRE
    );
    auto fire = create_node<ParticleSystem>(fire_script);
}
```

Nodes created this way are automatically tracked by the Scene. When you call `node->destroy()`, the node is queued for cleanup and will be fully destroyed after `late_update()`.

### Building a Node Hierarchy

StageNodes can be parented to each other to form a tree:

```cpp
void on_load() override {
    auto parent = create_node<Stage>();

    auto child1 = create_node<Actor>(mesh);
    child1->set_parent(parent);

    auto child2 = create_node<Actor>(mesh);
    child2->set_parent(parent);

    // Both children now move with the parent
    parent->transform->set_position(10, 0, 0);
}
```

For a complete guide to StageNodes, see [Stage Nodes](stage-nodes.md).

---

## The Relationship Between Scene and Stage

Understanding the relationship between `Scene` and `Stage` is important:

- **Scene** is the top-level container. It owns the asset manager, compositor, input manager reference, and the stage-node tree. It manages lifecycle (load/unload/activate/deactivate).
- **Stage** is a StageNode that acts as a grouping container. It has no visual representation of its own but is useful for organizing other nodes and for attaching render pipelines (Layers).

The inheritance chain looks like this:

```
Scene
  |-- StageNode (Scene IS-A stage node in the tree)
  |-- StageNodeManager (Scene CAN create and manage stage nodes)
  |-- StageNodeWatchController (Scene monitors node lifecycle)
```

A typical Scene contains one or more Stages, and each Stage contains Actors, Geoms, Cameras, and other nodes:

```
MyScene
  |-- Camera3D
  |-- DirectionalLight
  |-- Stage "world"
  |     |-- Actor "player"
  |     |-- Actor "enemy_1"
  |     |-- ParticleSystem "fire"
  |-- Stage "ui"
        |-- ui::Label "score"
        |-- ui::Button "pause"
```

### Render Pipelines (Layers)

A Layer connects a Camera to the Scene's compositor and defines how the Scene is rendered. You create Layers with `compositor->create_layer()`:

```cpp
void on_load() override {
    camera_ = create_node<Camera3D>();

    // Create a render pipeline for this camera
    pipeline_ = compositor->create_layer(this, camera_);
    pipeline_->set_background_color(Color(0.1f, 0.1f, 0.2f, 1.0f));
}
```

Only nodes that are part of a Stage attached to an active Layer will be rendered. See the [Scene Management](scene-management.md) documentation for details on multi-layer rendering and transitions.

---

## Best Practices for Organizing Game Logic Into Scenes

### 1. One Scene Per Screen

Each distinct screen or game state should be its own Scene class:

```
MenuScene       -> Main menu with buttons
GameScene       -> Core gameplay
LoadingScene    -> Loading screen with progress
SettingsScene   -> Options and configuration
GameOverScene   -> Game over with restart/quit buttons
```

### 2. Keep Scenes Self-Contained

A Scene should own everything it needs. Load assets through `assets->`, create nodes with `create_node<T>()`, and set up pipelines in `on_load()`. Avoid reaching into other Scenes or holding references to their nodes.

```cpp
// GOOD: Self-contained
void on_load() override {
    mesh_ = assets->load_mesh("level_1.glb");
    music_ = assets->load_sound("level1_music.ogg");
}

// BAD: Reaching outside the scene
void on_load() override {
    auto mesh = some_other_scene->mesh_;  // Don't do this
}
```

### 3. Use `on_load()` for Heavy Work, `on_activate()` for Runtime State

Load meshes, textures, and sounds in `on_load()`. Start music, reset timers, and spawn enemies in `on_activate()`. This separation lets you preload Scenes without starting their gameplay.

```cpp
void on_load() override {
    // Heavy asset loading
    hero_mesh_ = assets->load_mesh("hero.glb");
    level_mesh_ = assets->load_mesh("level.glb");
    music_ = assets->load_sound("battle.ogg");
}

void on_activate() override {
    // Runtime state
    score_ = 0;
    source_->play_sound(music_, AUDIO_REPEAT_LOOP, DISTANCE_MODEL_AMBIENT);
    spawn_enemies();
}
```

### 4. Use Background Loading for Smooth Transitions

For a seamless experience, preload the next Scene while the current one is still playing:

```cpp
void on_activate() override {
    // Preload the next level while the player is still in this one
    scenes->preload_in_background("next_level", level_number_ + 1);
}

void on_update(float dt) override {
    Scene::on_update(dt);

    // Check if loading is complete, then switch
    if (scenes->is_loaded("next_level")) {
        scenes->activate("next_level");
    }
}
```

### 5. Pass Arguments for Parameterized Scenes

Instead of creating `Level1Scene`, `Level2Scene`, etc., make one `GameScene` that takes a level parameter:

```cpp
class GameScene : public Scene {
public:
    GameScene(Window* window, int level)
        : Scene(window), level_(level) {}

    void on_load() override {
        std::string level_file = "level_" + std::to_string(level_) + ".glb";
        level_mesh_ = assets->load_mesh(level_file);
    }

private:
    int level_;
};

// Register with a specific level
scenes->register_scene<GameScene>("game", 3);
```

### 6. Clean Up Explicit Resources

While the engine handles StageNode cleanup automatically, you are responsible for destroying things like render pipelines (Layers) in `on_unload()`:

```cpp
void on_unload() override {
    if (pipeline_) {
        pipeline_->destroy();
        pipeline_ = nullptr;
    }
}
```

### 7. Use Services for Cross-Cutting Concerns

Scenes can start and stop lightweight services using `start_service<T>()` and `stop_service<T>()`. Use this for things like analytics, save-game managers, or online connectivity:

```cpp
void on_activate() override {
    auto save_manager = start_service<SaveManager>();
    save_manager->load_current_game();
}
```

---

## Example: Complete Scene Implementation

Here is a complete, production-quality Scene that demonstrates all the concepts covered in this document:

```cpp
#pragma once

#include <simulant/simulant.h>

using namespace smlt;

/**
 * GameScene -- A playable game level.
 *
 * This scene demonstrates:
 *   - Asset loading via the scene-local asset manager
 *   - Creating cameras, lights, actors, and UI
 *   - Setting up a render pipeline
 *   - Using all lifecycle methods correctly
 *   - Switching to another scene when the level is complete
 */
class GameScene : public Scene {
public:
    GameScene(Window* window, int level_number)
        : Scene(window),
          level_number_(level_number) {}

private:
    // -- State --
    int level_number_;
    float score_ = 0.0f;
    uint64_t level_start_time_ = 0;

    // -- Nodes --
    CameraPtr camera_;
    ui::Label* score_label_ = nullptr;

    // -- Pipeline --
    LayerPtr pipeline_;

    // -- Audio --
    SoundPtr background_music_;
    AudioSource* audio_source_ = nullptr;

    // -- on_load: Create everything --
    void on_load() override {
        S_INFO("GameScene: Loading level {}", level_number_);

        // Load level mesh
        std::string level_file = "levels/level_" + std::to_string(level_number_) + ".glb";
        auto level_mesh = assets->load_mesh(level_file);

        // Create the level actor
        auto level_actor = create_node<Actor>(level_mesh);
        level_actor->set_name("LevelGeometry");

        // Create a camera
        camera_ = create_node<Camera3D>();
        camera_->set_name("MainCamera");
        camera_->transform->set_position(0, 10, -15);
        camera_->look_at(0, 0, 0);

        // Create a light
        auto light = create_node<DirectionalLight>();
        light->set_color(Color(1.0f, 0.95f, 0.8f, 1.0f));
        light->set_direction(0, -1, 1);

        // Create audio source attached to camera
        audio_source_ = camera_->create_child<AudioSource>();
        background_music_ = assets->load_sound("music/battle.ogg");

        // Create UI
        score_label_ = create_node<ui::Label>("Score: 0");
        score_label_->set_anchor_point(0.0f, 1.0f);
        score_label_->set_position_2d(window->coordinate_from_normalized(0.05f, 0.05f));

        // Set up the render pipeline
        pipeline_ = compositor->create_layer(this, camera_);
        pipeline_->set_background_color(Color(0.1f, 0.1f, 0.2f, 1.0f));

        S_INFO("GameScene: Load complete");
    }

    // -- on_unload: Clean up explicit resources --
    void on_unload() override {
        S_INFO("GameScene: Unloading level {}", level_number_);

        if (pipeline_) {
            pipeline_->destroy();
            pipeline_ = nullptr;
        }
    }

    // -- on_activate: Start runtime state --
    void on_activate() override {
        S_INFO("GameScene: Activating level {}", level_number_);

        score_ = 0.0f;
        level_start_time_ = app->time_keeper->now_in_us();

        // Play background music
        if (audio_source_ && background_music_) {
            audio_source_->play_sound(
                background_music_,
                AUDIO_REPEAT_LOOP,
                DISTANCE_MODEL_AMBIENT
            );
        }
    }

    // -- on_deactivate: Pause runtime state --
    void on_deactivate() override {
        S_INFO("GameScene: Deactivating level {}", level_number_);

        if (audio_source_) {
            audio_source_->stop();
        }
    }

    // -- on_update: Frame-rate-dependent updates --
    void on_update(float dt) override {
        Scene::on_update(dt);

        // Update score display
        score_label_->set_text("Score: " + std::to_string((int)score_));

        // Handle input
        if (input->key_just_pressed(smlt::KEY_ESCAPE)) {
            scenes->activate("menu");
        }

        // Check if level is complete (example: after 60 seconds)
        float elapsed = (float)(app->time_keeper->now_in_us() - level_start_time_) / 1000000.0f;
        if (elapsed >= 60.0f) {
            S_INFO("Level complete! Score: {}", (int)score_);
            scenes->activate("game_over");
        }
    }

    // -- on_fixed_update: Physics/deterministic updates --
    void on_fixed_update(float step) override {
        Scene::on_fixed_update(step);

        // Physics simulation step (if you have one)
        // physics_world_->step(step);
    }

    // -- on_late_update: Post-update work --
    void on_late_update(float dt) override {
        Scene::on_late_update(dt);

        // Camera follow would go here, after the player has moved
        // camera_->transform->set_position(player_->position() + offset_);
    }
};
```

### Registering This Scene in the Application

```cpp
class MyGame : public Application {
public:
    MyGame(const AppConfig& config)
        : Application(config) {}

    bool init() override {
        // Register all scenes
        scenes->register_scene<MenuScene>("menu");
        scenes->register_scene<GameScene>("game", 1);    // Level 1
        scenes->register_scene<GameScene>("game_level_2", 2);  // Level 2
        scenes->register_scene<GameOverScene>("game_over");

        return true;
    }
};
```

---

## Quick Reference

| Concept | How To |
|---------|--------|
| Create a Scene | Subclass `Scene`, pass `Window*` to parent constructor |
| Register a Scene | `scenes->register_scene<MyScene>("name")` |
| Activate a Scene | `scenes->activate("name")` |
| Preload a Scene | `scenes->preload_in_background("name")` |
| Check if loaded | `scenes->is_loaded("name")` |
| Create a StageNode | `create_node<Camera3D>()`, `create_node<Actor>(mesh)`, etc. |
| Load an asset | `assets->load_texture("file.png")` |
| Create a pipeline | `compositor->create_layer(this, camera_)` |
| Switch scenes from within a Scene | `scenes->activate("other")` |
| Access the Application | `app->` |
| Access the Window | `window->` |
| Access input | `input->axis_value("name")` |

---

## See Also

- **[Scene Management](scene-management.md)** -- Scene transitions, preloading strategies, and activation behaviours
- **[Stage Nodes](stage-nodes.md)** -- Complete guide to StageNodes: hierarchy, traversal, destruction, and mixins
- **[Application](application.md)** -- Application lifecycle, AppConfig, and command-line arguments
- **[Cameras](../cameras.md)** -- Camera types and configuration
- **[Asset Managers](../asset_managers.md)** -- Loading and managing meshes, textures, sounds, and fonts
