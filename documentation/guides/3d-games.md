# 3D Game Development Guide

This guide covers techniques and best practices for building 3D games in Simulant. It assumes you have followed the [Complete Game Guide](complete-game.md) and understand the basics of scenes, actors, physics, and asset loading. Here we go deeper into 3D-specific topics.

---

## Table of Contents

1. [Scene and Camera Setup](#1-scene-and-camera-setup)
2. [Working with 3D Models](#2-working-with-3d-models)
3. [Lighting](#3-lighting)
4. [Materials and Shaders](#4-materials-and-shaders)
5. [Skeletal Animation](#5-skeletal-animation)
6. [Level Design](#6-level-design)
7. [Camera Systems](#7-camera-systems)
8. [Particle Systems](#8-particle-systems)
9. [Rendering Pipeline](#9-rendering-pipeline)
10. [Optimization for 3D](#10-optimization-for-3d)
11. [Platform-Specific Considerations](#11-platform-specific-considerations)

---

## 1. Scene and Camera Setup

### 3D Camera

Every 3D scene needs a `Camera3D` with perspective projection:

```cpp
class GameScene : public smlt::Scene {
private:
    smlt::CameraPtr camera_;
    smlt::LayerPtr pipeline_;

    void on_load() override {
        camera_ = create_child<smlt::Camera3D>();
        camera_->set_perspective_projection(
            smlt::Degrees(60.0f),    // Field of view
            window->aspect_ratio(),  // Aspect ratio
            0.1f,                    // Near plane
            1000.0f                  // Far plane
        );
        camera_->transform->set_position(0, 10, -15);
        camera_->look_at(smlt::Vec3(0, 0, 0));

        pipeline_ = compositor->create_layer(this, camera_);
        pipeline_->set_clear_flags(smlt::BUFFER_CLEAR_ALL);
        pipeline_->viewport->set_color(smlt::Color(0.4f, 0.6f, 0.9f, 1.0f));
    }
};
```

### Field of View Choices

| FOV | Use Case | Notes |
|-----|----------|-------|
| 45-60 degrees | Third-person, RPG | Natural perspective |
| 70-90 degrees | First-person, action | Wider view, slight distortion |
| 30-45 degrees | Isometric, strategy | Flatter perspective |
| 90+ degrees | Racing, flight sim | Extreme wide-angle |

### Multiple Cameras

Switch between cameras for cutscenes or different views:

```cpp
void switch_to_cinematic_camera() {
    cinematic_camera_->set_perspective_projection(
        smlt::Degrees(35.0f),
        window->aspect_ratio(),
        0.1f,
        500.0f
    );
    pipeline_->set_camera(cinematic_camera_);
}

void switch_to_game_camera() {
    pipeline_->set_camera(game_camera_);
}
```

---

## 2. Working with 3D Models

### Loading Models

Simulant's primary format is **glTF 2.0** (`.glb` / `.gltf`):

```cpp
auto mesh = assets->load_mesh("models/character.glb");
auto prefab = assets->load_prefab("models/character.glb");
```

The glTF loader handles:
- Skeletal animation (skinned and joint-based)
- PBR materials (diffuse, normal, metallic, roughness)
- Multiple submeshes with individual materials
- Automatic prefab creation

### Model Coordinate System

Simulant uses **Y-up, +Z forward**:
- **+Y** is up
- **+Z** points forward (into the screen by default)
- **+X** points right

Export models with **+Y Up** enabled. If your model appears rotated, re-export with the correct up-axis setting rather than applying a runtime rotation.

### Scale and Units

Use **1 unit = 1 meter** as your convention:

| Object | Approximate Size |
|--------|-----------------|
| Human character | 1.5 - 2.0 units tall |
| Door | 2.5 units tall, 1.0 unit wide |
| Room | 10 x 10 x 3 units |
| Car | 4.5 x 2.0 x 1.5 units |

### Submeshes and Materials

A model can contain multiple submeshes, each with its own material:

```cpp
auto mesh = assets->load_mesh("models/character.glb");

// Access individual submeshes
auto body_submesh = mesh->submesh("body");
auto head_submesh = mesh->submesh("head");

// Each submesh has 8 material slots
body_submesh->set_material_at_slot(smlt::MATERIAL_SLOT1, alt_material);

// Swap materials at runtime on an actor
auto actor = stage->new_actor_with_mesh(mesh);
actor->use_material_slot(smlt::MATERIAL_SLOT1);
```

### Texture Override

If your model references textures with the wrong extension:

```cpp
smlt::MeshLoadOptions opts;
opts.override_texture_extension = ".png";  // Model says .jpg but we have .png
auto mesh = assets->load_mesh("models/hero.obj", spec, opts);
```

---

## 3. Lighting

### Light Types

Simulant supports three light types:

| Type | Description | Use Case |
|------|-------------|----------|
| **DirectionalLight** | Infinite distance, parallel rays | Sun, moon |
| **PointLight** | Radiates in all directions from a point | Lamps, explosions, torches |
| **SpotLight** | Cone-shaped light with direction and angle | Flashlights, stage lights |

### Setting Up Lighting

```cpp
void create_lighting() {
    // Ambient light (base illumination)
    lighting->set_ambient_light(smlt::Color(0.3f, 0.3f, 0.4f, 1.0f));

    // Sun (directional)
    auto sun = create_child<smlt::DirectionalLight>();
    sun->set_color(smlt::Color(1.0f, 0.95f, 0.8f, 1.0f));
    sun->set_intensity(1.0f);
    sun->transform->set_rotation(
        smlt::Quaternion::angle_axis(smlt::Degrees(45), smlt::Vec3(1, 0, 0))
    );

    // Torch (point light)
    auto torch = create_child<smlt::PointLight>();
    torch->set_color(smlt::Color(1.0f, 0.6f, 0.2f, 1.0f));
    torch->set_intensity(0.8f);
    torch->set_attenuation(1.0f, 0.1f, 0.01f);  // Constant, linear, quadratic
    torch->transform->set_position(5, 3, 0);

    // Flashlight (spot light)
    auto flashlight = create_child<smlt::SpotLight>();
    flashlight->set_color(smlt::Color(1.0f, 1.0f, 1.0f, 1.0f));
    flashlight->set_intensity(1.0f);
    flashlight->set_cutoff_angle(smlt::Degrees(30));
    flashlight->set_exponent(2.0f);  // Falloff sharpness
    flashlight->transform->set_position(0, 2, 0);
    flashlight->transform->set_rotation(
        smlt::Quaternion::angle_axis(smlt::Degrees(-90), smlt::Vec3(1, 0, 0))
    );
}
```

### Light Limits

| Platform | Max Lights Per Object |
|----------|----------------------|
| Desktop | 8 |
| PSP | 4 |
| Dreamcast | 2 |

Lights are collected per-frame, culled by distance, and sorted by proximity. Only the closest N lights affect each renderable.

### Dynamic Light Attachment

Attach lights to moving objects:

```cpp
// Lantern carried by player
auto lantern_light = player_->create_child<smlt::PointLight>();
lantern_light->set_color(smlt::Color(1.0f, 0.7f, 0.3f, 1.0f));
lantern_light->set_intensity(0.5f);
lantern_light->transform->set_position(0.3f, 1.5f, 0.3f);  // Offset from player center
```

---

## 4. Materials and Shaders

### Creating Materials

```cpp
// Basic colored material
auto mat = assets->new_material();
mat->set_base_color(smlt::Color(0.8f, 0.2f, 0.2f, 1.0f));
mat->set_shininess(0.5f);

// Textured material
auto mat = assets->create_material_from_texture(texture);

// Multi-textured material with normal map
auto mat = assets->new_material();
mat->set_pass_count(1);
mat->pass(0)->set_diffuse_map(diffuse_texture);
mat->pass(0)->set_normal_map(normal_texture);
mat->pass(0)->set_specular_map(specular_texture);
```

### The .smat File Format

Material files are JSON documents:

```json
{
    "passes": [
        {
            "property_values": {
                "s_diffuse_map": "textures/brick.png",
                "s_normal_map": "textures/brick_normal.png",
                "s_material_diffuse": "1 0 0 1"
            }
        }
    ]
}
```

See the [Asset Pipeline Guide](asset-pipeline.md) for the full `.smat` specification.

### Custom Shaders (GL2X Renderer)

On desktop platforms using the GL 2.x renderer, you can use custom shaders:

```json
{
    "passes": [
        {
            "vertex_shader": "shaders/water.vert",
            "fragment_shader": "shaders/water.frag",
            "property_values": {
                "my_time_uniform": "0.0",
                "my_color_uniform": "0 0.5 1 1"
            }
        }
    ]
}
```

> **Note:** Custom shaders are **not supported** on Dreamcast (GL 1.x fixed-function renderer).

### Material Slots for Variants

Use material slots to swap appearances without creating new actors:

```cpp
// Create armor variants
auto hero_mat = assets->load_material("materials/hero_armor.smat");
auto hero_mat_gold = assets->load_material("materials/hero_armor_gold.smat");

mesh->submesh("chest")->set_material_at_slot(smlt::MATERIAL_SLOT1, hero_mat_gold);

// At runtime, switch between slots
actor->use_material_slot(smlt::MATERIAL_SLOT0);  // Default armor
actor->use_material_slot(smlt::MATERIAL_SLOT1);  // Gold armor
```

---

## 5. Skeletal Animation

### Loading and Playing Animations

When you load a glTF with animations as a prefab, an `AnimationController` is created automatically:

```cpp
auto prefab = assets->load_prefab("models/character.glb");
auto instance = prefab->instantiate(stage);

auto anim_controller = instance->find_mixin<smlt::AnimationController>();

// Play animations by name
anim_controller->play("idle", smlt::ANIMATION_LOOP_FOREVER);
anim_controller->play("walk", smlt::ANIMATION_LOOP_FOREVER);
anim_controller->play("attack", smlt::ANIMATION_LOOP_NONE);
```

### Animation Blending

Crossfade between animations smoothly:

```cpp
// Transition from idle to walk with a 0.2 second blend
anim_controller->play("walk", smlt::ANIMATION_LOOP_FOREVER, 0.2f);
```

### Animation Events

Use signals to trigger game events at specific animation frames:

```cpp
anim_controller->signal_animation_finished().connect(
    [this](const std::string& name) {
        if (name == "attack") {
            // Attack animation finished -- can act again
            can_attack_ = true;
        }
    }
);
```

### Important: Baking IK

Simulant does **not support** Inverse Kinematics (IK) at runtime. If your animation uses IK constraints, you must **bake IK into keyframes** before exporting from your DCC tool.

### Configuring Animation Ranges

For MS3D files, configure keyframe ranges before playing:

```cpp
auto mesh = assets->load_mesh("models/character.ms3d");
// Set animation keyframe ranges on the skeleton
```

---

## 6. Level Design

### Using Prefabs for Level Geometry

Build levels from GLB prefabs:

```cpp
void load_level(const std::string& level_path) {
    auto level_prefab = assets->load_prefab(level_path);
    auto level_instance = create_child<smlt::PrefabInstance>(level_prefab);

    // Create static collider for the level
    auto level_body = level_instance->create_mixin<smlt::StaticBody>();
    // Add colliders matching the level geometry
}
```

### Geom for Large Static Meshes

For large, static level geometry, use `Geom` instead of `Actor` to benefit from octree culling:

```cpp
auto level_mesh = assets->load_mesh("models/level_dungeon.glb");

smlt::GeomCullerOptions opts;
opts.type = smlt::GEOM_CULLER_TYPE_OCTREE;
opts.octree_max_depth = 5;
opts.octree_min_primitives = 10;

auto level_geom = create_child<smlt::Geom>(level_mesh->id(), smlt::Vec3::zero(), smlt::Quaternion::identity(), opts);
```

| Culler Type | Best For |
|-------------|----------|
| **Octree** | Enclosed 3D spaces (dungeons, buildings, interiors) |
| **Quadtree** | Terrain, flat outdoor environments |

### LOD (Level of Detail)

Set up multiple detail levels for distant objects:

```cpp
auto hero = create_child<smlt::Actor>();
hero->set_mesh(hero_high_id, smlt::DETAIL_LEVEL_NEAREST);
hero->set_mesh(hero_med_id, smlt::DETAIL_LEVEL_NEAR);
hero->set_mesh(hero_low_id, smlt::DETAIL_LEVEL_MID);
hero->set_mesh(hero_vlow_id, smlt::DETAIL_LEVEL_FAR);

// Configure distance thresholds on the pipeline
pipeline_->set_detail_level_distances(10.0f, 20.0f, 40.0f, 80.0f);
```

---

## 7. Camera Systems

### Third-Person Follow Camera

```cpp
smlt::Vec3 camera_offset_ = {0, 8, -12};
float camera_follow_speed_ = 5.0f;

void on_update(float dt) override {
    Scene::on_update(dt);

    if (player_ && camera_) {
        auto player_pos = player_->absolute_position();
        smlt::Vec3 target = player_pos + camera_offset_;

        // Smooth follow
        smlt::Vec3 current = camera_->absolute_position();
        smlt::Vec3 new_pos = current.lerp(target, camera_follow_speed_ * dt);

        camera_->transform->set_position(new_pos);
        camera_->look_at(player_pos);
    }
}
```

### First-Person Camera

```cpp
float pitch_ = 0;
float yaw_ = 0;

void on_update(float dt) override {
    Scene::on_update(dt);

    if (player_) {
        // Position camera at player's "head"
        auto player_pos = player_->absolute_position();
        camera_->transform->set_position(
            player_pos.x,
            player_pos.y + 1.6f,  // Eye height
            player_pos.z
        );

        // Rotation from mouse input
        float mouse_dx = input->mouse_delta_x();
        float mouse_dy = input->mouse_delta_y();

        yaw_ -= mouse_dx * 0.1f;
        pitch_ -= mouse_dy * 0.1f;
        pitch_ = std::clamp(pitch_, -89.0f, 89.0f);

        camera_->transform->set_rotation(
            smlt::Quaternion::angle_axis(smlt::Degrees(pitch_), smlt::Vec3(1, 0, 0)) *
            smlt::Quaternion::angle_axis(smlt::Degrees(yaw_), smlt::Vec3(0, 1, 0))
        );
    }
}
```

### Orbit Camera

```cpp
float orbit_distance_ = 15.0f;
float orbit_theta_ = 0;
float orbit_phi_ = smlt::Degrees(45);

void on_update(float dt) override {
    Scene::on_update(dt);

    if (target_) {
        auto center = target_->absolute_position();

        float x = orbit_distance_ * std::sin(orbit_phi_) * std::cos(orbit_theta_);
        float y = orbit_distance_ * std::cos(orbit_phi_);
        float z = orbit_distance_ * std::sin(orbit_phi_) * std::sin(orbit_theta_);

        camera_->transform->set_position(center.x + x, center.y + y, center.z + z);
        camera_->look_at(center);
    }
}
```

---

## 8. Particle Systems

### Loading Particle Scripts

Simulant uses the `.kglp` (or `.script`) format for particle definitions:

```cpp
auto particle_system = assets->load_particle_script("effects/fire.kglp");

auto emitter = create_child<smlt::ParticleSystem>(particle_system);
emitter->transform->set_position(0, 1, 0);
emitter->play();
```

### Creating Particles in Code

```cpp
auto particle_system = create_child<smlt::ParticleSystem>("Explosion");
particle_system->set_quota(100);
particle_system->set_particle_size(0.3f, 0.3f);
particle_system->set_material(particle_material_id);

// Add an emitter
auto emitter = particle_system->add_point_emitter();
emitter->set_direction(smlt::Vec3(0, 1, 0));
emitter->set_velocity(5.0f);
emitter->set_ttl(0.5f, 1.5f);
emitter->set_angle(30.0f);
emitter->set_color(smlt::Color(1.0f, 0.5f, 0.0f, 1.0f));
emitter->set_emission_rate(50);

// Add manipulators
auto size_manip = particle_system->add_size_manipulator();
size_manip->set_rate(-0.5f);  // Shrink over time

auto dir_manip = particle_system->add_direction_manipulator();
dir_manip->set_force(smlt::Vec3(0, 2.0f, 0));  // Upward force (like gravity reversed)
```

See the [Asset Pipeline Guide](asset-pipeline.md) for the full `.kglp` file format specification.

---

## 9. Rendering Pipeline

### Understanding Layers

A `Layer` connects a `Stage` and a `Camera` and produces rendered output. A scene can have multiple layers:

```cpp
// Main 3D game layer
game_pipeline_ = compositor->create_layer(this, camera_);
game_pipeline_->set_clear_flags(smlt::BUFFER_CLEAR_ALL);

// Minimap layer (rendered to a texture or viewport region)
minimap_pipeline_ = compositor->create_layer(this, minimap_camera_);
minimap_pipeline_->set_clear_flags(smlt::BUFFER_CLEAR_NONE);  // Overlay
```

### Render Priority

Layers are sorted by render priority. Use this to control draw order:

```cpp
// Background layer (skybox)
bg_pipeline_->set_render_priority(smlt::RENDER_PRIORITY_LOW);

// Main game layer
game_pipeline_->set_render_priority(smlt::RENDER_PRIORITY_DEFAULT);

// Foreground UI layer
ui_pipeline_->set_render_priority(smlt::RENDER_PRIORITY_FOREGROUND);
```

### Post-Processing

Layers can be configured with different clear flags, viewport colors, and render targets for effects like split-screen or picture-in-picture.

---

## 10. Optimization for 3D

### Actor vs Geom

| Feature | Actor | Geom |
|---------|-------|------|
| Movable | Yes | No |
| Per-frame overhead | Low | None |
| Culling | Frustum only | Octree/Quadtree |
| Best for | Characters, vehicles | Levels, terrain |

Use `Geom` for anything that does not move. The octree culling means only visible polygons are submitted to the GPU.

### Draw Call Batching

Simulant batches objects sharing the same material. Help the batcher:

```cpp
// GOOD: Shared material = fewer draw calls
auto crate_mat = assets->new_material();
crate_mat->set_diffuse_map(crate_tex);
crate_mat->set_garbage_collection_method(smlt::GARBAGE_COLLECT_NEVER);

for (int i = 0; i < 50; ++i) {
    auto crate = create_child<smlt::Actor>(crate_mesh_id);
    crate->set_material(crate_mat->id());
}
```

### Texture Optimization

| Platform | Max Texture Size | Power-of-Two |
|----------|-----------------|--------------|
| Desktop | GPU-dependent | No |
| Dreamcast | 1024px | Recommended |
| PSP | **512px** | **Required** |

- Use **DDS with DXT compression** on desktop for VRAM efficiency
- Use **DTEX/KMG** on Dreamcast (native KallistiOS formats)
- **Power-of-two** is mandatory on PSP

### Memory Management

> **Store `AssetID`, not `shared_ptr`.** Holding a `shared_ptr` as a class member prevents garbage collection.

```cpp
// GOOD
class Player {
    MeshID mesh_id_;

    void update() {
        {
            auto mesh = assets->mesh(mesh_id_);
            // Use mesh...
        }  // Pointer released here
    }
};
```

See [Performance Guide](performance.md) and [Resource Management](../core-concepts/resource-management.md) for details.

---

## 11. Platform-Specific Considerations

### Dreamcast

| Constraint | Value | Guidance |
|-----------|-------|----------|
| CPU | 200MHz SH-4 | Keep logic simple; reduce physics steps |
| RAM | 16MB | Use low-poly models; small textures |
| VRAM | 8MB | Texture atlases max 1024px |
| Max lights | 2 per object | Use ambient + 1 directional |
| Max texture | 1024px | Power-of-two recommended |
| Renderer | GL 1.x fixed-function | No custom shaders |
| Audio | OGG (streamed), WAV | Stream music; load SFX into memory |
| Assets | Embedded in ELF | All assets built into executable |

```cpp
#ifdef SIMULANT_PLATFORM_DREAMCAST
    // Use low-poly models
    auto mesh = assets->load_mesh("models/hero_dc.obj");

    // Minimal lighting: ambient + 1 directional only
    lighting->set_ambient_light(smlt::Color(0.5f, 0.5f, 0.5f, 1.0f));
    auto sun = create_child<smlt::DirectionalLight>();
    sun->set_intensity(1.0f);

    // No point lights, no spot lights (only 2 light slots available)

    // Reduce physics frequency
    physics_scene_->set_fixed_timestep(1.0f / 30.0f);

    // Avoid skyboxes -- use gradient background
    pipeline_->viewport->set_color(smlt::Color(0.4f, 0.6f, 0.9f, 1.0f));
#endif
```

### PSP

| Constraint | Value | Guidance |
|-----------|-------|----------|
| CPU | 333MHz MIPS | Similar to Dreamcast but slightly faster |
| RAM | 32MB | More headroom than Dreamcast |
| Max lights | 4 per object | Can use ambient + 2-3 point lights |
| Max texture | **512px** | **Power-of-two required** |
| Audio | WAV only | OGG not supported |
| Assets | Embedded | All assets built into executable |

```cpp
#ifdef SIMULANT_PLATFORM_PSP
    // Textures must be power-of-two, max 512px
    auto tex = assets->load_texture("textures/hero_256.png");

    // Use WAV audio
    auto music = assets->load_sound("music/level.wav");

    // Reduce detail for distant objects
    pipeline_->set_detail_level_distances(5.0f, 15.0f, 30.0f, 60.0f);
#endif
```

### Desktop

Desktop platforms have far fewer constraints. Focus on:
- Loading assets asynchronously to avoid hitches
- Using LOD systems effectively
- Using custom shaders for visual effects
- Streaming large levels in segments

---

## Summary

Key takeaways for 3D game development in Simulant:

1. **Use Camera3D** with perspective projection for 3D rendering
2. **Use glTF/GLB** as your primary model format -- it supports animation, materials, and prefabs
3. **Export models as Y-up** to match Simulant's coordinate system
4. **Use Geom for static geometry** to benefit from octree/quadtree culling
5. **Use Actor for moving objects** -- characters, vehicles, projectiles
6. **Share materials** across objects to enable draw call batching
7. **Be mindful of light limits** -- 8 on desktop, 4 on PSP, 2 on Dreamcast
8. **Store AssetIDs not shared_ptrs** to allow proper garbage collection
9. **Use platform-specific asset variants** for constrained platforms
10. **Preload assets** at scene start to avoid runtime loading hitches

---

## Further Reading

- [Asset Pipeline Guide](asset-pipeline.md) -- Preparing and loading 3D assets
- [Performance Guide](performance.md) -- Optimization techniques
- [Materials](../rendering/materials.md) -- Material properties and shaders
- [Textures](../rendering/textures.md) -- Texture loading and formats
- [Lighting](../rendering/lighting.md) -- Detailed lighting documentation
- [Mesh Formats](../assets/mesh-formats.md) -- Supported 3D file formats
- [Building a Complete Game](complete-game.md) -- Step-by-step tutorial
- [Particle Systems](../rendering/particle-systems.md) -- Visual effects
