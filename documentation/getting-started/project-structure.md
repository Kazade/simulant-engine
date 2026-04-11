# Project Structure

Understanding how Simulant projects are organized is crucial for effective game development. This guide explains every part of a Simulant project.

## Standard Project Layout

When you create a project with `simulant start mygame`, you get this structure:

```
mygame/
├── assets/              # Game assets (models, textures, sounds, materials)
│   ├── fonts/           # Font files
│   ├── materials/       # .smat material files
│   ├── meshes/          # 3D model files (.glb, .obj, etc.)
│   ├── particle_scripts/# Particle system definitions
│   ├── sounds/          # Audio files (.wav, .ogg)
│   └── textures/        # Image files (.png, .jpg, etc.)
├── libraries/           # Third-party libraries
├── packages/            # Built distributable packages
├── sources/             # C++ source code
│   └── main.cpp         # Entry point
├── tests/               # Unit test source code
├── tools/               # Binary tools (platform-specific)
├── CMakeLists.txt       # CMake build configuration
└── simulant.json        # Project metadata and settings
```

## Key Files

### `simulant.json`

This is your project configuration file. It contains:

```json
{
    "name": "mygame",
    "version": "0.1.0",
    "description": "My awesome game",
    "author": "Your Name",
    "target_platforms": ["linux", "dreamcast", "windows"],
    "asset_paths": ["assets"],
    "core_assets": true
}
```

**Important fields:**

- `name` - Your project name (used for executable name)
- `version` - Semantic version string
- `target_platforms` - Which platforms to build for
- `asset_paths` - Directories to search for assets (can have multiple)
- `core_assets` - Whether to include Simulant's built-in assets

### `CMakeLists.txt`

This is the CMake build configuration. You typically won't need to edit this unless you're adding external libraries.

The default file sets up:
- C++17 standard
- Simulant library linking
- Asset embedding for target platforms
- Test executable generation

## The Assets Directory

The `assets/` directory is where all your game content goes. Simulant's asset system will search this directory (and any paths listed in `asset_paths`) when loading files.

### Asset Loading

Assets are loaded using **paths relative to asset search paths**:

```cpp
// Loads from assets/meshes/character.glb
auto mesh = assets->load_mesh("meshes/character.glb");

// Loads from assets/textures/wood.png
auto texture = assets->load_texture("textures/wood.png");

// Loads from assets/sounds/explosion.ogg
auto sound = assets->load_sound("sounds/explosion.ogg");
```

### Asset Subdirectories

While you can organize assets however you like, these conventions are common:

```
assets/
├── meshes/          # 3D models
│   ├── characters/  # Character models
│   ├── environments/# Level geometry
│   └── props/       # Interactive objects
├── textures/        # Textures
│   ├── characters/  # Character textures
│   ├── environments/# Level textures
│   └── ui/          # UI images
├── materials/       # Material definitions (.smat files)
├── sounds/          # Sound effects and music
│   ├── sfx/         # Sound effects
│   └── music/       # Background music
├── fonts/           # Font files
└── particle_scripts/# Particle effects
```

## The Sources Directory

The `sources/` directory contains your C++ code. A typical structure looks like:

```
sources/
├── main.cpp              # Application entry point
├── application.h         # Your Application subclass
├── application.cpp
├── scenes/               # Scene classes
│   ├── menu_scene.h
│   ├── menu_scene.cpp
│   ├── game_scene.h
│   └── game_scene.cpp
├── actors/               # Custom Actor subclasses (if needed)
├── behaviours/           # Reusable Behaviour components
├── ui/                   # UI-related code
└── utils/                # Utility functions
```

### Entry Point: main.cpp

Every Simulant game starts with a `main()` function:

```cpp
#include <simulant/simulant.h>
#include "application.h"

int main(int argc, char* argv[]) {
    smlt::AppConfig config;
    config.title = "My Game";
    config.width = 1280;
    config.height = 720;
    
    MyApp app(config);
    return app.run(argc, argv);
}
```

### Application Class

Your Application subclass (often in `application.h/cpp`) is where you:

- Register scenes
- Configure global settings
- Handle application-level logic

```cpp
#pragma once
#include <simulant/simulant.h>

class MyApp : public smlt::Application {
public:
    MyApp(const smlt::AppConfig& config):
        Application(config) {}

    bool init() override;
};
```

## The Tests Directory

The `tests/` directory contains unit tests for your game logic:

```
tests/
├── main.cpp              # Test runner entry point
├── test_game_logic.h     # Test cases
└── test_physics.h
```

Run tests with:

```bash
simulant test
```

See the [Testing Guide](../reference/testing.md) for details.

## Build Outputs

When you build your project, outputs go into platform-specific directories:

```
build/
└── linux/
    ├── mygame            # Executable
    └── tests             # Test executable
```

## Multi-Platform Considerations

Simulant projects are designed to work across platforms. Key considerations:

### Asset Formats

Different platforms support different file formats:

| Format | Linux | Windows | Dreamcast | PSP |
|--------|-------|---------|-----------|-----|
| PNG    | ✅    | ✅      | ✅        | ✅  |
| JPG    | ✅    | ✅      | ✅        | ✅  |
| OGG    | ✅    | ✅      | ✅        | ❌  |
| WAV    | ✅    | ✅      | ✅        | ✅  |
| GLB    | ✅    | ✅      | ⚠️ Limited| ⚠️ Limited |

### Build Targets

You can build for different platforms:

```bash
# Linux (native)
simulant build

# Dreamcast (requires Docker)
simulant build dreamcast

# Windows (requires Docker)
simulant build windows
```

### Conditional Compilation

Use platform macros when needed:

```cpp
#ifdef SIMULANT_PLATFORM_DREAMCAST
    // Dreamcast-specific code
#elif defined(SIMULANT_PLATFORM_PSP)
    // PSP-specific code
#else
    // Desktop code
#endif
```

## Asset Embedding

For platforms like Dreamcast that don't have a filesystem, Simulant **embeds assets** into the executable during the build process. This is handled automatically by the build system.

On desktop platforms, assets are loaded from disk at runtime, making iteration faster during development.

## Next Steps

- **[Your First Game](first-game.md)** - If you haven't already
- **[Application](../core-concepts/application.md)** - Learn about the Application class
- **[Scenes](../core-concepts/scenes.md)** - Understand scene management
- **[Asset Management](../assets/asset-managers.md)** - Working with assets effectively
