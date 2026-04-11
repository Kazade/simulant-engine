# Scene Management

The `SceneManager` is the engine subsystem responsible for creating, loading, activating, deactivating, and unloading scenes. It handles the entire lifecycle of scene transitions, including passing arguments between scenes, preloading in the background, and managing activation behaviour.

**Related documentation:**
- [Scenes](scenes.md) -- Creating scenes, lifecycle methods, and stage node management
- [Application](application.md) -- Application entry point and accessing the SceneManager
- [Render Pipelines](../rendering/pipelines.md) -- Setting up layers, multi-camera rendering, and transitions
- [Signals](../scripting/signals.md) -- Using scene activation/deactivation signals

---

## 1. SceneManager Overview

The `SceneManager` is a property of the `Application` class, accessible from anywhere in your code via the `scenes` shortcut. Its job is to:

- Store **scene factories** -- recipes for creating scenes on demand
- Instantiate scenes the first time they are requested
- Load and activate scenes with configurable behaviour
- Pass arguments to scenes at load time
- Preload scenes synchronously or in the background
- Provide signals when scenes activate or deactivate

At a high level, the SceneManager works with two internal maps:

| Map | Purpose |
|-----|---------|
| `scene_factories_` | Stores the factory functions registered by `register_scene<T>()` |
| `routes_` | Stores already-instantiated `Scene` objects, keyed by their string name |

When you call `activate("game")`, the manager looks up `"game"` in `routes_`. If it is not there, it uses the factory from `scene_factories_` to create it. This means scenes are created **lazily** -- they are not instantiated until they are first needed.

```
Application
  |
  +-- SceneManager
        |
        +-- scene_factories_ : { "menu" => MenuScene factory,
        |                        "game" => GameScene factory, ... }
        |
        +-- routes_ : { "menu" => MenuScene instance (active),
        |               "game" => GameScene instance (loaded but not active) }
        |
        +-- current_scene_ => MenuScene (the active scene)
```

### Accessing the SceneManager

From anywhere in your Application or a Scene, the SceneManager is available via the `scenes` property:

```cpp
// From within a Scene
scenes->activate("menu");
scenes->preload("loading");

// From within the Application
scenes->register_scene<MenuScene>("menu");
scenes->activate("menu");

// From anywhere using the global app pointer
smlt::get_app()->scenes->activate("game");
```

---

## 2. Scene Registration

Before a scene can be activated, it must be registered with the SceneManager. Registration is typically done in your Application's `init()` method.

### Basic Registration

The simplest form registers a scene type with no extra arguments:

```cpp
class MyGame : public Application {
public:
    MyGame(const AppConfig& config)
        : Application(config) {}

    bool init() override {
        scenes->register_scene<MenuScene>("menu");
        scenes->register_scene<GameScene>("game");
        scenes->register_scene<GameOverScene>("game_over");
        return true;
    }
};
```

The string `"main"` is special: it is the first scene the engine attempts to activate when the application starts. Make sure one scene is registered with this name, or manually activate a different scene yourself.

### Registration with Constructor Arguments

If your Scene constructor takes additional parameters beyond `Window*`, you can pass them at registration time:

```cpp
// Scene definition
class GameScene : public Scene {
public:
    GameScene(Window* window, int level_number, bool hard_mode)
        : Scene(window),
          level_number_(level_number),
          hard_mode_(hard_mode) {}

private:
    int level_number_;
    bool hard_mode_;

    void on_load() override {
        S_INFO("Loading level {} (hard_mode={})", level_number_, hard_mode_);
    }
};

// Registration -- the extra args are forwarded to the constructor
scenes->register_scene<GameScene>("game", 3, true);
```

You can pass any number of arguments. They are captured and stored, then used each time the scene factory creates an instance.

### Registering Multiple Factory Variants

Because the factory is keyed by a string name, you can register the same Scene type under different names with different parameters:

```cpp
scenes->register_scene<GameScene>("level_1", 1);
scenes->register_scene<GameScene>("level_2", 2);
scenes->register_scene<GameScene>("level_3", 3);
```

### Unregistering a Scene

You can remove a scene factory at any time with `unregister_scene()`. This also unloads and destroys any existing instance of that scene:

```cpp
scenes->unregister_scene("level_1");
```

After unregistering, calling `activate("level_1")` will throw an error because no factory exists for that name.

---

## 3. Scene Lifecycle

Every Scene goes through a well-defined lifecycle managed by the SceneManager. Understanding this lifecycle is essential to writing correct scene code.

### Lifecycle States and Transitions

```
                  register_scene()
                        |
                        v
                   [Registered]  <-- Factory stored, scene not yet created
                        |
                   activate() / preload()
                        |
                        v
                   [Loading]  <-- on_pre_load() -> on_load()
                        |
                        v
                  [Activated]  <-- on_activate()
                        |
                        |  (every frame)
                        v
                  [Updating]   <-- on_update(dt), on_fixed_update(step), on_late_update(dt)
                        |
                   activate(other_scene)
                        |
                        v
                 [Deactivated] <-- on_deactivate()
                        |
               (depends on behaviour)
                  /          \
                 v            v
         [Unloaded]       [Still Loaded]
      on_unload()      (ready to re-activate)
      on_post_unload()
```

### Lifecycle Methods

These are the methods you can override in your Scene subclass, in the order they are called:

| Method | When Called |
|--------|-------------|
| `on_pre_load()` | Before `on_load()` -- optional pre-load hook |
| `on_load()` | **Pure virtual** -- create nodes, load assets, set up pipelines |
| `on_activate()` | When the scene becomes the active scene |
| `on_update(float dt)` | Every frame while active |
| `on_fixed_update(float step)` | At a fixed timestep (60 Hz) while active |
| `on_late_update(float dt)` | After all `on_update` calls, while active |
| `on_deactivate()` | When the scene stops being active |
| `on_unload()` | When the scene is unloaded |
| `on_post_unload()` | After `on_unload()` -- optional post-unload hook |

For detailed information on each method, see the [Scenes](scenes.md) documentation.

### Manual Load and Unload

You can also control the load state independently of activation:

```cpp
// Check if a scene is loaded
if (scenes->is_loaded("game")) {
    S_INFO("Game scene is already loaded");
}

// Manually unload a scene
scenes->unload("game");

// Unload and destroy all scenes
scenes->destroy_all();
scenes->clean_destroyed_scenes();

// Reset -- unloads and clears everything
scenes->reset();
```

Calling `unload()` on a scene will also call `deactivate()` first if the scene is currently active.

---

## 4. Scene Activation Behaviours

When switching from one scene to another, the SceneManager supports two activation behaviours that control the order of unload and load operations.

### `ACTIVATE_BEHAVIOUR_UNLOAD_FIRST` (Default)

This is the **default** behaviour. When you call `activate()`:

1. The current scene is **deactivated** (`on_deactivate()` called)
2. The current scene is **unloaded** (if `unload_on_deactivate` is true)
3. The new scene is **loaded** (if not already loaded, `on_load()` called)
4. The new scene is **activated** (`on_activate()` called)

```
Current Scene                New Scene
     |                            |
  deactivate()                    |
  unload()                        |
                             load()
                             activate()
```

Use this when you want to free memory from the current scene before loading the next one. This is the safest choice for memory-constrained platforms.

```cpp
scenes->activate("menu", ACTIVATE_BEHAVIOUR_UNLOAD_FIRST);
// Or simply (this is the default):
scenes->activate("menu");
```

### `ACTIVATE_BEHAVIOUR_UNLOAD_AFTER`

This behaviour loads the new scene **before** unloading the old one:

1. The new scene is **loaded** (if not already loaded)
2. The current scene is **deactivated**
3. The scenes are **swapped** (new scene becomes active)
4. The new scene is **activated**
5. The current scene is **unloaded** (if `unload_on_deactivate` is true)

```
Current Scene                New Scene
     |                            |
                             load()
  deactivate()                    |
      <--- swap --->              |
                             activate()
                             unload() (old scene)
```

Use this when you need the new scene to be fully loaded before the old one is torn down. This is useful for:

- **Transition effects** -- keep the old scene's render pipeline alive while fading in the new scene
- **Seamless transitions** -- have both scenes coexist briefly during the transition

```cpp
scenes->activate("game", ACTIVATE_BEHAVIOUR_UNLOAD_AFTER);
```

### Controlling Unload on Deactivate

By default, scenes are set to `unload_on_deactivate = true`, meaning they are automatically unloaded when deactivated. You can change this per scene:

```cpp
class PersistentHUDScene : public Scene {
public:
    PersistentHUDScene(Window* window)
        : Scene(window) {
        // This scene should stay loaded across transitions
        set_unload_on_deactivate(false);
    }

    void on_load() override {
        // Set up HUD elements that persist
    }
};
```

When `unload_on_deactivate` is `false`, the scene stays loaded in memory and can be re-activated instantly without going through `on_load()` again. Only `on_activate()` will be called.

---

## 5. Preloading Scenes

Preloading allows you to load a scene's assets ahead of time so that activation is instant. There are two ways to preload: **synchronous** and **asynchronous (background)**.

### Synchronous Preload: `preload()`

Loads the scene immediately on the calling thread:

```cpp
class MyGame : public Application {
public:
    bool init() override {
        scenes->register_scene<GameScene>("game");

        // Preload the game scene synchronously
        scenes->preload("game", 1);  // Passes level 1 as a constructor arg

        // Now activate it -- no loading delay
        scenes->activate("game");

        return true;
    }
};
```

If the scene is already loaded, `preload()` returns immediately without doing anything.

### Asynchronous Preload: `preload_in_background()`

Loads the scene on a background thread, returning a `Promise<void>` that you can use to know when loading is complete:

```cpp
class MyGame : public Application {
public:
    bool init() override {
        scenes->register_scene<LoadingScene>("loading");
        scenes->register_scene<GameScene>("game");

        // Show loading screen immediately
        scenes->activate("loading");

        // Start loading the game scene in the background
        scenes->preload_in_background("game", 1).then([this]() {
            // This callback runs when loading is complete
            scenes->activate("game");
        });

        return true;
    }
};
```

The `.then()` method starts a **coroutine** that waits until the preload finishes, then executes your callback. This is the recommended pattern for any scene that takes noticeable time to load.

### Checking Load State

You can check if a scene is loaded at any time:

```cpp
if (scenes->is_loaded("game")) {
    scenes->activate("game");
} else {
    // Still loading -- show a progress indicator
    loading_bar_->set_progress(0.5f);
}
```

### Checking if Activation is Pending

You can also check if a scene activation has been queued but not yet executed:

```cpp
if (scenes->scene_queued_for_activation()) {
    S_INFO("A scene activation is pending");
}
```

This is useful in the Application's `init()` to avoid double-activating:

```cpp
bool init() override {
    scenes->register_scene<MenuScene>("menu");

    // Only activate if no activation is already queued
    if (!scenes->scene_queued_for_activation()) {
        scenes->activate("menu");
    }
    return true;
}
```

---

## 6. Passing Arguments to Scenes

There are two ways to pass data to scenes: **constructor arguments** (at registration time) and **load-time arguments** (at activation time).

### Constructor Arguments (Registration Time)

As shown earlier, constructor arguments are passed when the scene factory creates the scene instance. These are fixed at registration time and cannot change between activations:

```cpp
// Registered once with a fixed level number
scenes->register_scene<GameScene>("level1", 1);

// Every time level1 is activated, it uses level_number = 1
scenes->activate("level1");
scenes->activate("menu");
scenes->activate("level1");  // Same level_number = 1
```

### Load-Time Arguments (Activation Time)

Any arguments passed to `activate()`, `preload()`, or `preload_in_background()` beyond the scene name are stored in the scene's `load_args` vector. The scene can access them via `get_load_arg<T>(index)`:

```cpp
class GameScene : public Scene {
public:
    GameScene(Window* window)
        : Scene(window) {}

    void on_load() override {
        // Read load-time arguments
        if (load_arg_count() >= 2) {
            int level = get_load_arg<int>(0);
            std::string difficulty = get_load_arg<std::string>(1);
            S_INFO("Starting level {} on {} difficulty", level, difficulty);
        }
    }
};
```

Pass the arguments when activating or preloading:

```cpp
// Activate with load-time arguments
scenes->activate("game", 3, std::string("hard"));

// Preload with load-time arguments
scenes->preload("game", 5, std::string("easy"));

// Background preload with arguments
scenes->preload_in_background("game", 2, std::string("normal")).then([this]() {
    scenes->activate("game");
});
```

### When to Use Each Approach

| Use Case | Approach |
|----------|----------|
| Fixed properties that never change (e.g. base level number at registration) | Constructor arguments |
| Dynamic values that vary per activation (e.g. player score carried over) | Load-time arguments |
| A single scene type reused for many levels with different params | Constructor arguments per registration name |
| Runtime state passed between scenes (e.g. "coming from level 3") | Load-time arguments |

---

## 7. Scene Transitions and Transition Effects

Scene transitions are the moment the player moves from one scene to another. The SceneManager provides signals and activation behaviours that make it possible to implement smooth, visually pleasing transitions.

### Using Activation Signals

The SceneManager fires two signals during every scene change:

```cpp
// Connect in your Application or a dedicated transition manager
scenes->signal_scene_activated().connect(
    [this](const std::string& route, Scene* scene) {
        S_INFO("Scene activated: {}", route);
        on_scene_entered(route);
    }
);

scenes->signal_scene_deactivated().connect(
    [this](const std::string& route, Scene* scene) {
        S_INFO("Scene deactivated: {}", route);
        on_scene_exited(route);
    }
);
```

### Implementing Fade Transitions

A common transition effect is a fade-to-black. This is typically done using a full-screen black UI element whose opacity is animated. The recommended approach is to use the `ACTIVATE_BEHAVIOUR_UNLOAD_AFTER` behaviour so that both the old and new scene pipelines remain active during the transition.

```cpp
class TransitionManager {
public:
    TransitionManager(Application* app)
        : app_(app) {

        app->scenes->signal_scene_deactivated().connect(
            [this](const std::string&, Scene*) {
                start_fade_out();
            }
        );

        app->scenes->signal_scene_activated().connect(
            [this](const std::string&, Scene*) {
                start_fade_in();
            }
        );
    }

private:
    Application* app_;
    ui::Image* fade_overlay_ = nullptr;

    void start_fade_out() {
        // Animate a black overlay to opacity 1.0 over 0.5 seconds
        // Implementation depends on your animation system
    }

    void start_fade_in() {
        // Animate the black overlay from opacity 1.0 to 0.0 over 0.5 seconds
    }
};
```

### Using a Loading Scene as a Transition

A common pattern is to activate a loading scene, preload the target in the background, and then activate the target when ready. The built-in `Splash` scene demonstrates this pattern:

```cpp
bool init() override {
    scenes->register_scene<Splash>("splash", std::string("menu"));
    scenes->register_scene<MenuScene>("menu");
    scenes->register_scene<GameScene>("game");

    // Splash is the "main" scene -- it will auto-transition to "menu"
    // No explicit activate needed, the engine activates "main" automatically
    return true;
}
```

### Cross-Fade Between Scenes

For a cross-fade (old scene fades out while new scene fades in behind it):

```cpp
void crossfade_to(const std::string& target) {
    cr_async([this, target]() {
        // 1. Start fading out the current scene's overlay alpha
        fade_overlay(1.0f, 0.5f);
        cr_yield_for(0.5f);

        // 2. Activate with UNLOAD_AFTER so both scenes are briefly active
        app->scenes->activate(target, ACTIVATE_BEHAVIOUR_UNLOAD_AFTER);

        // 3. Fade in the new scene
        fade_overlay(0.0f, 0.5f);
        cr_yield_for(0.5f);
    });
}
```

---

## 8. Overlay Scenes (Global HUD, Pause Menus)

The Application provides a special `overlay` scene that is **always rendered** with the highest priority over all other scenes. This is designed for persistent UI elements like:

- FPS counters and debug information
- Performance graphs
- Game version text
- Global HUD elements that must appear on top of every scene

### How the Overlay Works

The overlay scene is created automatically by the Application when the render context is established. It is an internal `OverlayScene` subclass of `Scene` that is always initialized and always rendered.

```cpp
// The overlay scene is accessible via the `overlay` property
auto overlay = app->overlay;
```

### Adding Elements to the Overlay

Because the overlay is a regular Scene, you can create StageNodes inside it:

```cpp
class MyGame : public Application {
public:
    MyGame(const AppConfig& config)
        : Application(config) {}

    bool init() override {
        scenes->register_scene<MenuScene>("menu");
        scenes->register_scene<GameScene>("game");

        // Add a persistent FPS counter to the overlay
        auto fps_label = overlay->create_node<ui::Label>("FPS: 60");
        fps_label->set_anchor_point(1.0f, 1.0f);
        fps_label->set_position_2d(
            window->coordinate_from_normalized(0.98f, 0.98f)
        );

        // Connect to the frame signal to update it
        signal_frame_finished().connect([this, fps_label]() {
            float fps = time_keeper->fps();
            fps_label->set_text("FPS: " + std::to_string((int)fps));
        });

        return true;
    }
};
```

### Using the Overlay for a Pause Menu

A pause menu can be implemented by adding UI nodes to the overlay scene and toggling their visibility:

```cpp
class PauseMenu {
public:
    PauseMenu(Application* app) {
        auto overlay = app->overlay;

        resume_btn_ = overlay->create_node<ui::Button>("Resume");
        quit_btn_ = overlay->create_node<ui::Button>("Quit");

        // Initially hidden
        set_visible(false);
    }

    void show() {
        resume_btn_->set_visible(true);
        quit_btn_->set_visible(true);
        paused_ = true;
    }

    void hide() {
        resume_btn_->set_visible(false);
        quit_btn_->set_visible(false);
        paused_ = false;
    }

private:
    ui::Button* resume_btn_ = nullptr;
    ui::Button* quit_btn_ = nullptr;
    bool paused_ = false;
};
```

### Overlay Rendering Priority

The overlay scene is always rendered last (highest priority), so anything you add to it will appear on top of all scene content. This makes it ideal for HUDs that must always be visible regardless of which scene is active.

For more on render priority, see [Render Pipelines](../rendering/pipelines.md).

---

## 9. Background Loading Scenes

Background loading is the pattern where you show a loading screen while the next scene loads in the background. This prevents the player from staring at a frozen screen.

### Basic Background Loading Pattern

```cpp
class MyGame : public Application {
public:
    bool init() override {
        scenes->register_scene<LoadingScene>("loading");
        scenes->register_scene<MenuScene>("menu");

        // Show loading screen immediately
        scenes->activate("loading");

        // Load menu in the background
        scenes->preload_in_background("menu").then([this]() {
            scenes->activate("menu");
        });

        return true;
    }
};
```

### Loading Scene with Progress

A loading scene can monitor the load state of the target scene and display progress:

```cpp
class LoadingScene : public Scene {
public:
    LoadingScene(Window* window, std::string target)
        : Scene(window), target_(std::move(target)) {}

private:
    std::string target_;
    ui::ProgressBar* progress_bar_ = nullptr;
    ui::Label* status_label_ = nullptr;

    void on_load() override {
        auto camera = create_node<Camera2D>();
        camera->set_orthographic_projection(
            0, window->width(), 0, window->height()
        );

        pipeline_ = compositor->create_layer(this, camera);

        progress_bar_ = create_node<ui::ProgressBar>(0.0f);
        progress_bar_->set_anchor_point(0.5f, 0.5f);
        progress_bar_->set_position_2d(
            window->coordinate_from_normalized(0.5f, 0.5f)
        );

        status_label_ = create_node<ui::Label>("Loading...");
        status_label_->set_anchor_point(0.5f, 0.4f);
        status_label_->set_position_2d(
            window->coordinate_from_normalized(0.5f, 0.45f)
        );
    }

    void on_update(float dt) override {
        Scene::on_update(dt);

        // Check if the target scene has finished loading
        if (scenes->is_loaded(target_)) {
            status_label_->set_text("Ready!");
            progress_bar_->set_progress(1.0f);

            // Activate the target scene
            scenes->activate(target_);
            return;
        }

        // Update progress (simulated -- you'd need a way to track actual progress)
        progress_bar_->set_progress(std::min(progress_bar_->get_progress() + dt * 0.3f, 0.95f));
    }

    void on_unload() override {
        if (pipeline_) {
            pipeline_->destroy();
        }
    }

    LayerPtr pipeline_;
};
```

### Using the Promise `.then()` Chain

The `preload_in_background()` method returns a `Promise<void>`, which can be chained:

```cpp
scenes->preload_in_background("level_2").then([this]() {
    S_INFO("Level 2 is loaded");
    scenes->activate("level_2");
});
```

Multiple preloads can be queued:

```cpp
scenes->preload_in_background("level_1").then([this]() {
    scenes->preload_in_background("level_2");
    scenes->preload_in_background("level_3");
    scenes->activate("level_1");
});
```

---

## 10. Scene Persistence Across Transitions

By default, scenes are unloaded when they are deactivated. However, you may want certain scenes to persist across transitions.

### `unload_on_deactivate` Property

Every scene has an `unload_on_deactivate` flag (default: `true`). When set to `false`, the scene remains loaded after deactivation:

```cpp
class GlobalMusicScene : public Scene {
public:
    GlobalMusicScene(Window* window)
        : Scene(window) {
        // Keep this scene loaded even when deactivated
        set_unload_on_deactivate(false);
    }

    void on_load() override {
        // Load music assets and set up audio
        music_ = assets->load_sound("background_music.ogg");
        source_ = create_node<AudioSource>();
    }

    void on_activate() override {
        source_->play_sound(music_, AUDIO_REPEAT_LOOP, DISTANCE_MODEL_AMBIENT);
    }

    void on_deactivate() override {
        // Don't stop the music -- let it keep playing
        // The scene stays loaded in memory
    }

private:
    SoundPtr music_;
    AudioSource* source_ = nullptr;
};
```

### Re-activating a Persistent Scene

When you re-activate a scene that was not unloaded, only `on_activate()` is called -- `on_load()` is skipped because the scene is already loaded:

```cpp
// First activation: on_load() -> on_activate()
scenes->activate("music");

// Deactivating (e.g., by activating another scene):
// on_deactivate() is called, but NOT on_unload()

// Re-activating: only on_activate() is called
scenes->activate("music");  // Fast -- no loading needed
```

### Manually Controlling Persistence

You can also manually control persistence by toggling the flag at runtime:

```cpp
// Make the current scene persist
scenes->active_scene()->set_unload_on_deactivate(true);

// Later, when you want to free it:
scenes->active_scene()->set_unload_on_deactivate(false);
```

---

## 11. Managing Scenes with Pipelines

Scenes and render pipelines are tightly coupled. Each scene typically creates one or more `Layer` objects through its `compositor`. Understanding how pipelines interact with scene transitions is important for advanced rendering.

### Pipeline Lifecycle

When a scene is unloaded, you are responsible for destroying its pipelines:

```cpp
void on_unload() override {
    if (pipeline_) {
        pipeline_->destroy();
        pipeline_ = nullptr;
    }
}
```

Pipelines are not destroyed automatically because they may need to outlive the scene (e.g., during transitions).

### Multiple Pipelines Per Scene

A single scene can have multiple pipelines for different rendering purposes:

```cpp
void on_load() override {
    // Main 3D camera
    camera3d_ = create_node<Camera3D>();

    // Main 3D pipeline
    main_pipeline_ = compositor->create_layer(this, camera3d_);
    main_pipeline_->set_background_color(Color(0.1f, 0.1f, 0.2f, 1.0f));

    // 2D UI camera
    camera2d_ = create_node<Camera2D>();
    camera2d_->set_orthographic_projection(0, window->width(), 0, window->height());

    // UI pipeline with higher priority (draws on top)
    ui_pipeline_ = compositor->create_layer(this, camera2d_);
    ui_pipeline_->set_priority(RENDER_PRIORITY_FOREGROUND);
}
```

### Keeping Pipelines Active During Transitions

When using `ACTIVATE_BEHAVIOUR_UNLOAD_AFTER`, the old scene's pipelines remain active while the new scene loads and activates. This allows you to implement effects like:

```cpp
void on_activate() override {
    // Use UNLOAD_AFTER to keep the previous scene's pipeline briefly active
    scenes->activate("game", ACTIVATE_BEHAVIOUR_UNLOAD_AFTER);

    // At this point, both old and new scenes may have active pipelines
    // The compositor will render both, sorted by priority
}
```

### Off-Screen Render Targets

Scenes can render to off-screen textures for effects like mirrors, security cameras, or picture-in-picture:

```cpp
void on_load() override {
    // Create a texture to render into
    mirror_texture_ = window->create_texture(256, 256);

    // Create a pipeline that renders to the texture instead of the screen
    auto mirror_pipeline = compositor->create_layer(
        this,
        mirror_camera_,
        Viewport(0, 0, 1.0f, 1.0f),  // Full viewport
        mirror_texture_                // Off-screen target
    );
}
```

For a complete guide to pipelines and layers, see [Render Pipelines](../rendering/pipelines.md).

---

## 12. Complete Examples

### Example 1: Menu to Game Transition

A complete flow from main menu to gameplay with proper loading:

```cpp
// --- MenuScene ---
class MenuScene : public Scene {
public:
    MenuScene(Window* window)
        : Scene(window) {}

private:
    LayerPtr pipeline_;
    CameraPtr camera_;
    ui::Button* play_button_ = nullptr;

    void on_load() override {
        camera_ = create_node<Camera2D>();
        camera_->set_orthographic_projection(
            0, window->width(), 0, window->height()
        );

        pipeline_ = compositor->create_layer(this, camera_);
        pipeline_->set_background_color(Color(0.0f, 0.0f, 0.0f, 1.0f));

        // Title
        auto title = create_node<ui::Label>("My Awesome Game");
        title->set_anchor_point(0.5f, 0.8f);
        title->set_position_2d(
            window->coordinate_from_normalized(0.5f, 0.8f)
        );

        // Play button
        play_button_ = create_node<ui::Button>("Play");
        play_button_->set_anchor_point(0.5f, 0.5f);
        play_button_->set_position_2d(
            window->coordinate_from_normalized(0.5f, 0.5f)
        );
    }

    void on_update(float dt) override {
        Scene::on_update(dt);

        if (play_button_->is_pressed()) {
            // Start loading the game scene in background
            scenes->preload_in_background("game", 1).then([this]() {
                scenes->activate("game");
            });
        }
    }

    void on_unload() override {
        if (pipeline_) {
            pipeline_->destroy();
        }
    }
};

// --- GameScene ---
class GameScene : public Scene {
public:
    GameScene(Window* window, int level)
        : Scene(window), level_(level) {}

private:
    int level_;
    LayerPtr pipeline_;
    CameraPtr camera_;

    void on_load() override {
        S_INFO("Loading level {}", level_);

        // Load level mesh
        std::string mesh_file = "levels/level_" + std::to_string(level_) + ".glb";
        auto mesh = assets->load_mesh(mesh_file);
        create_node<Actor>(mesh);

        camera_ = create_node<Camera3D>();
        camera_->transform->set_position(0, 10, -15);

        pipeline_ = compositor->create_layer(this, camera_);
    }

    void on_activate() override {
        S_INFO("Level {} activated", level_);
    }

    void on_deactivate() override {
        S_INFO("Level {} deactivated", level_);
    }

    void on_update(float dt) override {
        Scene::on_update(dt);

        // Press Escape to return to menu
        if (input->key_just_pressed(KEY_ESCAPE)) {
            scenes->activate("menu");
        }
    }

    void on_unload() override {
        if (pipeline_) {
            pipeline_->destroy();
        }
    }
};

// --- Application ---
class MyGame : public Application {
public:
    MyGame(const AppConfig& config)
        : Application(config) {}

    bool init() override {
        scenes->register_scene<MenuScene>("menu");
        scenes->register_scene<GameScene>("game", 1);

        // Menu is "main", so it activates automatically
        return true;
    }
};
```

### Example 2: Level Loading with Arguments

Demonstrates passing level number and difficulty at activation time:

```cpp
class GameScene : public Scene {
public:
    GameScene(Window* window)
        : Scene(window) {}

private:
    int current_level_ = 0;
    std::string difficulty_ = "normal";

    LayerPtr pipeline_;
    CameraPtr camera_;

    void on_load() override {
        // Read the arguments passed at activation time
        if (load_arg_count() >= 2) {
            current_level_ = get_load_arg<int>(0);
            difficulty_ = get_load_arg<std::string>(1);
        }

        S_INFO("Loading level {} on {} difficulty", current_level_, difficulty_);

        std::string mesh_file = "levels/level_" + std::to_string(current_level_) + ".glb";
        auto mesh = assets->load_mesh(mesh_file);
        create_node<Actor>(mesh);

        camera_ = create_node<Camera3D>();
        camera_->transform->set_position(0, 10, -15);
        pipeline_ = compositor->create_layer(this, camera_);
    }

    void on_activate() override {
        // Start preloading the next level
        int next_level = current_level_ + 1;
        scenes->preload_in_background("game", next_level, difficulty_)
            .then([this, next_level]() {
                S_INFO("Level {} is preloaded", next_level);
            });
    }

    void on_update(float dt) override {
        Scene::on_update(dt);

        // Right arrow goes to next level
        if (input->key_just_pressed(KEY_RIGHT)) {
            int next = current_level_ + 1;
            scenes->activate("game", next, difficulty_);
        }

        // Left arrow goes to previous level
        if (input->key_just_pressed(KEY_LEFT) && current_level_ > 1) {
            int prev = current_level_ - 1;
            scenes->activate("game", prev, difficulty_);
        }

        // Escape returns to menu
        if (input->key_just_pressed(KEY_ESCAPE)) {
            scenes->activate("menu");
        }
    }

    void on_unload() override {
        if (pipeline_) {
            pipeline_->destroy();
        }
    }
};
```

### Example 3: Background Loading with Loading Screen

A complete background loading pattern with progress display:

```cpp
// --- LoadingScene ---
class LoadingScene : public Scene {
public:
    LoadingScene(Window* window, std::string target)
        : Scene(window), target_(std::move(target)) {}

private:
    std::string target_;
    LayerPtr pipeline_;
    CameraPtr camera_;
    ui::Label* status_label_ = nullptr;

    void on_load() override {
        camera_ = create_node<Camera2D>();
        camera_->set_orthographic_projection(
            0, window->width(), 0, window->height()
        );

        pipeline_ = compositor->create_layer(this, camera_);
        pipeline_->set_background_color(Color(0.05f, 0.05f, 0.1f, 1.0f));

        status_label_ = create_node<ui::Label>("Loading...");
        status_label_->set_anchor_point(0.5f, 0.5f);
        status_label_->set_position_2d(
            window->coordinate_from_normalized(0.5f, 0.5f)
        );
    }

    void on_activate() override {
        status_label_->set_text("Loading " + target_ + "...");
    }

    void on_update(float dt) override {
        Scene::on_update(dt);

        if (scenes->is_loaded(target_)) {
            status_label_->set_text("Ready!");
            scenes->activate(target_);
        }
    }

    void on_unload() override {
        if (pipeline_) {
            pipeline_->destroy();
        }
    }
};

// --- Application ---
class MyGame : public Application {
public:
    MyGame(const AppConfig& config)
        : Application(config) {}

    bool init() override {
        scenes->register_scene<LoadingScene>("loading", std::string("game"));
        scenes->register_scene<GameScene>("game");
        scenes->register_scene<MenuScene>("menu");

        // The engine will activate "main" automatically.
        // If "main" is not registered, activate "loading" manually:
        // scenes->activate("loading");

        return true;
    }
};
```

### Example 4: Physics Sample Pattern (from samples/boxes.cpp)

A common pattern used in the engine's own samples:

```cpp
class PhysicsDemo : public Application {
public:
    PhysicsDemo(const AppConfig& config)
        : Application(config) {}

private:
    bool init() override {
        scenes->register_scene<GameScene>("main");

        // Show a loading screen while the main scene loads in background
        scenes->activate("_loading");

        scenes->preload_in_background("main").then([this]() {
            // Loading complete -- switch to main
            scenes->activate("main");
        });

        return true;
    }
};
```

---

## Quick Reference

| Task | Code |
|------|------|
| Register a scene | `scenes->register_scene<MyScene>("name")` |
| Register with constructor args | `scenes->register_scene<GameScene>("game", 3, true)` |
| Activate (default behaviour) | `scenes->activate("name")` |
| Activate with UNLOAD_FIRST | `scenes->activate("name", ACTIVATE_BEHAVIOUR_UNLOAD_FIRST)` |
| Activate with UNLOAD_AFTER | `scenes->activate("name", ACTIVATE_BEHAVIOUR_UNLOAD_AFTER)` |
| Activate with load-time args | `scenes->activate("name", arg1, arg2)` |
| Preload synchronously | `scenes->preload("name")` |
| Preload in background | `scenes->preload_in_background("name").then([&]() { ... })` |
| Check if loaded | `scenes->is_loaded("name")` |
| Check if activation pending | `scenes->scene_queued_for_activation()` |
| Unload a scene | `scenes->unload("name")` |
| Unregister a scene | `scenes->unregister_scene("name")` |
| Destroy all scenes | `scenes->destroy_all()` |
| Reset everything | `scenes->reset()` |
| Get active scene | `scenes->active_scene()` |
| Set unload on deactivate | `scene->set_unload_on_deactivate(false)` |
| Scene activation signal | `scenes->signal_scene_activated().connect(...)` |
| Scene deactivation signal | `scenes->signal_scene_deactivated().connect(...)` |
| Access overlay | `app->overlay` |
| Read load arg in scene | `get_load_arg<int>(0)` |
| Count load args | `load_arg_count()` |

---

## See Also

- **[Scenes](scenes.md)** -- Scene lifecycle methods, properties, and creating stage nodes
- **[Application](application.md)** -- Application entry point, AppConfig, and accessing the SceneManager
- **[Render Pipelines](../rendering/pipelines.md)** -- Layers, compositor, render priority, and multi-camera setups
- **[Signals](../scripting/signals.md)** -- Scene transition signals and event-driven programming
- **[Coroutines](../coroutines.md)** -- Using `cr_async()` and `Promise::then()` for background loading
