# Application

The `Application` class is the entry point to every Simulant game. It manages the game lifecycle, initializes subsystems, and provides access to core services.

## Overview

Every Simulant game must have a class that inherits from `Application`. This class:

- Initializes the engine subsystems (rendering, audio, input, etc.)
- Manages the `Window` and its configuration
- Provides access to the `SceneManager`
- Handles the main game loop
- Exposes signals for lifecycle events

## Creating an Application

### Basic Structure

```cpp
#include <simulant/simulant.h>

using namespace smlt;

class MyGame : public Application {
public:
    MyGame(const AppConfig& config):
        Application(config) {}

    bool init() override {
        // Register scenes and configure your game
        scenes->register_scene<MenuScene>("menu");
        scenes->register_scene<GameScene>("main");
        return true;
    }
};

int main(int argc, char* argv[]) {
    AppConfig config;
    config.title = "My Game";
    config.width = 1280;
    config.height = 720;
    
    MyGame app(config);
    return app.run(argc, argv);
}
```

### The init() Method

The `init()` method is called after the engine is initialized but before the first frame. This is where you:

- **Register scenes** - Define available scenes for your game
- **Configure settings** - Set up game-wide configuration
- **Initialize services** - Create custom services or managers

You **must** register at least one scene with the name `"main"` - this will be the first scene loaded.

### Lifecycle Methods

Application provides several methods you can override:

| Method | When Called | Purpose |
|--------|-------------|---------|
| `init()` | After engine init | Register scenes, configure |
| `update(dt)` | Every frame | Game-level update before scenes |
| `fixed_update(dt)` | Fixed timestep (60Hz) | Physics-related updates |
| `late_update(dt)` | After scene updates | Post-frame updates |
| `shutdown()` | Before exit | Cleanup resources |

## AppConfig

The `AppConfig` structure controls how your application starts:

### Window Settings

```cpp
AppConfig config;
config.title = "My Game";           // Window title
config.width = 1280;                // Window width
config.height = 720;                // Window height
config.fullscreen = false;          // Fullscreen mode
```

### Development Settings

```cpp
// Force a specific renderer
config.development.force_renderer = "gl2x";  // or "gl1x"

// Force a specific sound driver
config.development.force_sound_driver = "openal";

// Log to file
config.development.log_file = "game.log";

// Enable profiling
config.development.force_profiling = true;
```

### Virtual Screen (for Dreamcast VMU testing)

```cpp
config.desktop.enable_virtual_screen = true;
config.desktop.virtual_screen_width = 48;
config.desktop.virtual_screen_height = 32;
config.desktop.virtual_screen_integer_scale = true;
```

## Application Signals

Signals provide hooks into the application lifecycle. These are useful for system-wide monitoring or debugging:

```cpp
// Frame lifecycle signals
app->signal_frame_started().connect([]() {
    // Called at start of each frame
});

app->signal_update().connect([](float dt) {
    // Called during update phase
});

app->signal_late_update().connect([](float dt) {
    // Called after all updates
});

app->signal_frame_finished().connect([]() {
    // Called after frame is complete
});

// Shutdown signal
app->signal_shutdown().connect([]() {
    // Called when application is exiting
});
```

## Accessing Core Services

From within your Application (or any Scene), you can access:

### Window

The `Window` manages rendering, input, and audio:

```cpp
window->set_title("New Title");
window->set_cursor_visible(false);
window->screen_count();  // Number of connected screens
```

### SceneManager

Manage scenes:

```cpp
scenes->register_scene<MyScene>("scene_name");
scenes->load_and_activate("scene_name");
```

### Asset Managers

Access shared and local assets:

```cpp
// Shared assets (global)
window->shared_assets()->load_texture("texture.png");

// Scene-local assets (from within a scene)
assets->load_mesh("model.glb");
```

### Input Manager

Handle user input:

```cpp
input->new_axis("jump");
input->axis_value("jump");
```

### Sound Driver

Play sounds that persist across scenes:

```cpp
auto playing = application->sound_driver->play_sound(music_sound);
```

## Command Line Arguments

Simulant provides built-in argument parsing:

```cpp
class MyGame : public Application {
public:
    MyGame(const AppConfig& config):
        Application(config) {
        
        // Define custom arguments
        args->define_arg("--debug", ARG_TYPE_BOOLEAN, "Enable debug mode");
        args->define_arg("--level", ARG_TYPE_INTEGER, "Start at level N");
    }

    bool init() override {
        // Access arguments
        if (auto debug = args->arg_value<bool>("debug", false)) {
            S_INFO("Debug mode enabled");
        }
        
        auto level = args->arg_value<int>("level", 1);
        S_INFO("Starting at level: {}", level.value());
        
        return true;
    }
};
```

Run with arguments:

```bash
./mygame --debug --level 5
./mygame --help  # Shows available arguments
```

## Time Management

Access timing information:

```cpp
// Delta time (time since last frame)
float dt = time_keeper->dt();

// Frames per second
float fps = time_keeper->fps();

// Total elapsed time
float elapsed = time_keeper->elapsed();
```

## Best Practices

### 1. Keep Application Lightweight

Don't load heavy assets in `init()`. Let scenes manage their own assets.

```cpp
bool init() override {
    // Good: Just register scenes
    scenes->register_scene<GameScene>("main");
    
    // Bad: Loading assets here
    // auto mesh = assets->load_mesh("huge_model.glb");
}
```

### 2. Use Scene Registration Names Wisely

Choose descriptive names for scenes:

```cpp
scenes->register_scene<MenuScene>("menu");
scenes->register_scene<Level1Scene>("level_1");
scenes->register_scene<GameScene>("main");
```

The name `"main"` is special - it's the first scene loaded.

### 3. Handle Shutdown Gracefully

If you allocate resources that need cleanup:

```cpp
void shutdown() override {
    // Clean up custom resources
    my_custom_manager.cleanup();
    Application::shutdown();
}
```

### 4. Use Signals for Debugging

Frame signals are great for debugging:

```cpp
signal_frame_started().connect([]() {
    // Reset per-frame debug state
});

signal_post_coroutines().connect([]() {
    // Check coroutine state
});
```

## Example: Complete Application

```cpp
#include <simulant/simulant.h>
#include "scenes/menu_scene.h"
#include "scenes/game_scene.h"
#include "scenes/settings_scene.h"

using namespace smlt;

class MyGame : public Application {
public:
    MyGame(const AppConfig& config):
        Application(config) {
        
        // Define command line arguments
        args->define_arg("--skip-intro", ARG_TYPE_BOOLEAN, "Skip intro cinematic");
        args->define_arg("--god-mode", ARG_TYPE_BOOLEAN, "Enable god mode");
    }

    bool init() override {
        // Register all scenes
        scenes->register_scene<MenuScene>("menu");
        scenes->register_scene<GameScene>("game");
        scenes->register_scene<SettingsScene>("settings");
        scenes->register_scene<GameScene>("main");
        
        // Access command line arguments
        if (args->arg_value<bool>("god-mode", false)) {
            S_INFO("God mode enabled!");
            GameScene::god_mode = true;
        }
        
        // Set up frame debugging
        signal_frame_finished().connect([]() {
            // Could log frame timing here
        });
        
        return true;
    }

    void update(float dt) override {
        // Application-level update (runs before scene update)
        Application::update(dt);
    }
};

int main(int argc, char* argv[]) {
    AppConfig config;
    config.title = "My Awesome Game";
    config.width = 1920;
    config.height = 1080;
    
    MyGame app(config);
    return app.run(argc, argv);
}
```

## See Also

- **[Scenes](scenes.md)** - Managing game states
- **[Scene Management](scene-management.md)** - Transitions and lifecycle
- **[Project Structure](../getting-started/project-structure.md)** - Application in project context
- **[AppConfig Reference](../reference/environment-variables.md)** - Environment variables and config
