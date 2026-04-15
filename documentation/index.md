# Simulant Game Engine Documentation

Welcome to the Simulant Game Engine documentation! This comprehensive guide will help you build games using Simulant, from your first application to advanced features.

Simulant is a cross-platform C++ game engine with support for desktop (Linux, Windows), retro consoles (Sega Dreamcast), and handhelds (PSP). It features a scene graph architecture, compositor-based rendering, physics simulation, UI system, and more.

---

## 🚀 Getting Started

**New to Simulant? Start here!**

- **[Overview](getting-started/overview.md)** - What is Simulant and what can it do?
- **[Installation Guide](getting-started/installation.md)** - Get set up on Fedora, Ubuntu, or Windows (WSL2)
- **[Your First Game](getting-started/first-game.md)** - Complete beginner tutorial
- **[Project Structure](getting-started/project-structure.md)** - Understanding Simulant projects
- **[Development Setup](getting-started/ide-setup.md)** - Configure your IDE and tools

---

## 📚 Core Concepts

**The fundamental building blocks of Simulant games.**

- **[Application](core-concepts/application.md)** - Entry point to your game
- **[Scenes](core-concepts/scenes.md)** - Managing game states and screens
- **[Scene Management](core-concepts/scene-management.md)** - Transitions, preloading, and scene lifecycle
- **[Stage & StageNodes](core-concepts/stage-nodes.md)** - The scene graph hierarchy
- **[Actors](core-concepts/actors.md)** - Mesh-rendering entities in your scene
- **[Cameras](core-concepts/cameras.md)** - Viewpoints into your 3D world
- **[Transforms & Hierarchy](core-concepts/transforms.md)** - Positioning, rotation, and scaling
- **[Resource Management](core-concepts/resource-management.md)** - IDs, references, and garbage collection

---

## 🎨 Rendering System

**Everything about rendering visuals in Simulant.**

- **[Render Pipelines](rendering/pipelines.md)** - Compositor and render passes
- **[Materials](rendering/materials.md)** - Controlling appearance with materials
- **[Textures](rendering/textures.md)** - Loading and using textures
- **[Meshes](rendering/meshes.md)** - 3D geometry and mesh handling
- **[Mesh Instancer](rendering/mesh-instancer.md)** - Efficient instanced rendering
- **[Lighting](rendering/lighting.md)** - Point lights, directional lights, and illumination
- **[Sprites](rendering/sprites.md)** - 2D sprite rendering
- **[Particle Systems](rendering/particle-systems.md)** - GPU particle effects
- **[Viewports & Layers](rendering/viewports-layers.md)** - Multi-camera setups
- **[Partitioners & Culling](rendering/partitioners.md)** - Spatial optimization
- **[Geom & Static Geometry](rendering/geom.md)** - Optimized static meshes

---

## 🎵 Audio System

- **[Audio Overview](audio/overview.md)** - Playing sounds and music
- **[Positional Audio](audio/positional.md)** - 3D sound and spatialization
- **[Audio Formats](audio/formats.md)** - Supported sound file formats

---

## 🎮 Input System

- **[Input Overview](input/overview.md)** - Reading keyboards, mice, and gamepads
- **[Input Axes](input/axes.md)** - Configurable input abstraction
- **[Rumble & Feedback](input/rumble.md)** - Controller feedback

---

## ⚙️ Physics System

- **[Physics Overview](physics/overview.md)** - Introduction to physics simulation
- **[Rigid Bodies](physics/rigid-bodies.md)** - Dynamic, static, and kinematic bodies
- **[Colliders & Fixtures](physics/colliders.md)** - Shapes and collision detection
- **[Joints](physics/joints.md)** - Connecting physics bodies
- **[Raycasting](physics/raycasting.md)** - Collision queries
- **[Physics Best Practices](physics/best-practices.md)** - Optimization and common patterns

---

## 🖥️ UI System

- **[UI Overview](ui/overview.md)** - Introduction to the widget system
- **[Widgets](ui/widgets.md)** - Labels, buttons, images, and more
- **[Layouts & Frames](ui/layouts.md)** - Organizing UI elements
- **[Styling & Themes](ui/styling.md)** - Customizing appearance
- **[Text & Localization](ui/localization.md)** - Multi-language support

---

## 🎬 Animation

- **[Animation Overview](animation/overview.md)** - Animation system introduction
- **[Skeleton Animation](animation/skeleton-animation.md)** - Rigged character animation
- **[Animation Controller](animation/animation-controller.md)** - Playing and blending animations
- **[Sprite Animation](animation/sprite-animation.md)** - 2D sprite sheets

---

## 🛠️ Scripting & Advanced Patterns

- **[Coroutines](coroutines.md)** - Asynchronous game logic
- **[Lua Scripting](lua-scripting.md)** - Define StageNode behaviours in Lua
- **[Signals & Events](scripting/signals.md)** - Event-driven programming
- **[Behaviours](scripting/behaviours.md)** - Reusable node components
- **[Threading](scripting/threading.md)** - Background processing

---

## 📦 Assets & Resources

- **[Asset Managers](assets/asset-managers.md)** - Loading and managing game assets
- **[Virtual File System](assets/vfs.md)** - File search paths and organization
- **[Prefab System](assets/prefabs.md)** - Reusable scene templates
- **[Mesh Formats](assets/mesh-formats.md)** - Supported 3D file formats
- **[Material Files](assets/material-files.md)** - .smat material format
- **[Particle Script Format](assets/particle-script.md)** - Particle system definitions

---

## 🧰 Utilities

- **[Math Library](utilities/math.md)** - Vectors, matrices, quaternions
- **[Random Numbers](utilities/random.md)** - PRNG utilities
- **[JSON Parsing](utilities/json.md)** - Reading JSON files
- **[Logging](utilities/logging.md)** - Debug output and diagnostics
- **[Debug Drawing](utilities/debug-drawing.md)** - Visualization helpers
- **[Profiling](utilities/profiling.md)** - Performance measurement
- **[Command Line Args](utilities/arg-parsing.md)** - Runtime configuration

---

## 📖 Guides

- **[Building a Complete Game](guides/complete-game.md)** - Step-by-step game development
- **[2D Game Development](guides/2d-games.md)** - Tips for 2D games
- **[3D Game Development](guides/3d-games.md)** - Tips for 3D games
- **[Dreamcast Development](guides/dreamcast.md)** - Sega Dreamcast specifics
- **[PSP Development](guides/psp.md)** - PSP specifics
- **[Performance Optimization](guides/performance.md)** - Making your game run faster
- **[Asset Pipeline](guides/asset-pipeline.md)** - Importing and preparing assets
- **[Packaging & Distribution](guides/packaging.md)** - Shipping your game

---

## 📝 Tutorials

- **[Tutorial 1: Basic Application](tutorials/01_basic_application.md)** - Your first Simulant app
- **[Tutorial 2: Loading 3D Models](tutorials/02_loading_models.md)** - Working with meshes and prefabs
- **[Tutorial 3: Adding Physics](tutorials/03_physics.md)** - Physics simulation basics
- **[Tutorial 4: Building a UI](tutorials/04_user_interface.md)** - Creating menus and HUDs
- **[Tutorial 5: Animation](tutorials/05_animation.md)** - Character animation

---

## 🔧 Development

- **[C++ Guidelines](reference/cpp-guidelines.md)** - Code style and best practices
- **[Engine Structure](reference/engine-structure.md)** - Internal architecture
- **[Testing Framework](reference/testing.md)** - Writing unit tests
- **[Debugging Tips](reference/debugging.md)** - Debugging strategies
- **[Platform Notes](reference/platform-notes.md)** - Cross-platform considerations
- **[Environment Variables](reference/environment-variables.md)** - Runtime configuration
- **[Known Issues & TODO](reference/known-issues.md)** - Current limitations

---

## 📚 API Reference

For detailed API documentation, see the [Doxygen-generated reference](https://simulant.gitlab.io/simulant/namespacesmlt.html).

---

## 🤝 Contributing

Simulant is an open-source project. See [CONTRIBUTING.md](../CONTRIBUTING.md) for information on how to contribute.
