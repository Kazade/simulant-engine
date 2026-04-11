# Simulant Overview

Simulant is a cross-platform C++ game engine designed to make game development accessible, flexible, and fun. Originally created as a solo project, Simulant has grown into a feature-rich engine capable of producing full games for multiple platforms.

## What is Simulant?

Simulant is a **scene graph-based game engine** written in modern C++ (C++17). It provides everything you need to build a game:

- **Rendering**: 3D and 2D rendering with a flexible compositor-based pipeline
- **Physics**: Rigid body physics simulation with colliders, joints, and raycasting
- **Audio**: Positional 3D audio with support for WAV and OGG formats
- **Input**: Unified input abstraction for keyboard, mouse, and gamepads
- **UI**: A widget-based UI system with layout support
- **Animation**: Skeleton animation for characters, plus 2D sprite sheets
- **Particles**: GPU-accelerated particle systems for visual effects
- **Cross-platform**: Write once, deploy to Linux, Windows, Dreamcast, and PSP

## Key Design Principles

### Scene Graph Architecture

Everything in Simulant is organized as a **tree of StageNodes**. This includes:

- **Actors** - Objects that render meshes
- **Cameras** - Viewpoints into the scene
- **Lights** - Sources of illumination
- **Particle Systems** - Visual effects
- **UI Widgets** - Interface elements
- **Physics Bodies** - Physical objects

This tree structure makes it natural to think about parent-child relationships. For example, a sword Actor can be a child of a character's hand Actor, and automatically follow its movement.

### Compositor-Based Rendering

Simulant uses a **Compositor** system to manage rendering. You create **Layers** (or **Pipelines**) that define:

- What part of the scene to render (a Stage or subtree)
- Which Camera to render from
- Where to render to (screen, texture, etc.)
- In what order layers composite together

This makes it trivial to create effects like:
- A 3D world with a 2D UI overlay
- Picture-in-picture (CCTV cameras, minimaps)
- Post-processing effects
- Split-screen multiplayer

### ID-Based Resource Management

Simulant uses an **ID system** for resources like meshes, textures, and materials. You work with IDs (`MeshID`, `TextureID`, etc.) rather than raw pointers, which:

- Prevents dangling pointers
- Enables automatic garbage collection
- Makes it easy to check if a resource still exists
- Reduces memory leaks

### Scene-Based Game Structure

Games in Simulant are organized into **Scenes**. Each Scene represents a distinct part of your game:

- Main menu Scene
- Gameplay Scene (one per level)
- Loading Scene
- Game over Scene

Scenes have a clear lifecycle (`load`, `activate`, `update`, `deactivate`, `unload`) and can pass data to each other during transitions.

## Supported Platforms

| Platform | Status | Notes |
|----------|--------|-------|
| **Linux (Fedora, Ubuntu, etc.)** | ✅ Primary | Best development experience |
| **Windows (via WSL2)** | ✅ Supported | Cross-compilation via Docker |
| **Sega Dreamcast** | ✅ Supported | Requires KOS toolchain |
| **PSP** | ⚠️ Experimental | Limited by hardware |

## System Requirements

### Development (Linux)

- **OS**: Fedora 29+ or Ubuntu 18.04+
- **Compiler**: GCC with C++17 support
- **Build System**: CMake 3.10+
- **Graphics**: OpenGL 2.0+ (OpenGL 1.4 fallback available)
- **Audio**: OpenAL
- **Optional**: Docker (for cross-platform builds)

### Minimum Hardware

Simulant is designed to run on modest hardware, including retro consoles:

- **Dreamcast**: 200MHz CPU, 16MB RAM
- **PSP**: 333MHz CPU, 32MB RAM
- **PC**: Any modern system will exceed requirements

## Engine Architecture

At a high level, Simulant is structured as follows:

```
Application
├── Window (graphics, input, audio)
├── SceneManager
│   └── Scene (your game logic)
│       ├── Stage (scene graph root)
│       │   ├── Actor (renders meshes)
│       │   ├── Camera (viewpoint)
│       │   ├── Light (illumination)
│       │   └── ... (other StageNodes)
│       ├── AssetManager (loads assets)
│       ├── Compositor (render pipelines)
│       └── Services (physics, etc.)
├── SoundDriver (audio playback)
└── InputManager (input handling)
```

## What Can You Build?

Simulant is suitable for a wide range of game types:

- **3D Adventures**: Third-person or first-person games
- **2D Platformers**: Side-scrolling or top-down games
- **Puzzle Games**: Logic and spatial puzzles
- **Retro Games**: Native Dreamcast or PSP titles
- **Prototypes**: Rapid game concept testing

## Next Steps

- **[Installation](installation.md)** - Get Simulant set up
- **[Your First Game](first-game.md)** - Build a simple game from scratch
- **[Project Structure](project-structure.md)** - Understand how Simulant projects are organized

## Community & Support

- **Discord**: Join the Simulant Discord server for help and discussion
- **GitLab**: Report issues and contribute code on GitLab
- **Doxygen**: [API Reference](https://simulant.gitlab.io/simulant/namespacesmlt.html)
