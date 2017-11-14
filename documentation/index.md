# Simulant Documentation

## First Steps

 - Installation: [Fedora](install_fedora.md)
 - Tutorial: [1. Basic Application](tutorial_1.md) | [2. Rendering Pipelines](tutorial_2.md) | [3. Actors and Meshes](tutorial_3.md) | [4. Keyboard Input](tutorial_4.md)
 - Platforms: [Platform Notes](platform_notes.md)
 - Licensing: [FAQ](license.md)

## Architecture

### Basic Concepts

This section covers the base subsystems and usages of Simulant.

 - Overview: [Engine Structure](engine_structure.md) | [Resource Management](resource_management.md)
 - Managing objects: [Manual managers](manual_managers.md) | [Refcounted managers](refcount_managers.md)
 - Core systems: [The Window](window.md) | [Idle Manager](idle.md) | [Viewports](viewport.md) | [Threading](threading.md)
 - Utilities: [Random](random.md) 

### The Rendering System

 - The rendering process: [The Render Sequence](render_sequence.md) | [Pipelines](pipeline.md)
 - Partitioning: [Overview](partitioners.md) | [Spatial Hash Partitioner](spatial_hashing.md) | [The Null Partitioner](null_partitioner.md)
 - User interfaces: [Widgets](widgets.md)
 - Cameras: [Cameras](cameras.md)

### Scene Building

 - Scene Management: [The Scene](scene.md) | [Managing Scenes](scene_management.md)
 - Behaviours: [Organisms and Behaviours](behaviours.md)
 - Helpers: [Backgrounds](backgrounds.md)
 - Scripting: [Particle System File Format](particle_system_format.md)
 - Scene Tree: [Stage Nodes](stage_nodes.md)
 - Supported File Formats: [Mesh Formats](mesh_formats.md) | [Image Formats](image_formats.md)
 - Assets: [Textures](textures.md) | [Meshes](meshes.md)

