# 2D Game Development Guide

This guide covers techniques and best practices for building 2D games in Simulant. Whether you are making a platformer, top-down RPG, puzzle game, or arcade shooter, this guide will help you use Simulant's systems effectively for 2D development.

---

## Table of Contents

1. [Overview: 2D in Simulant](#1-overview-2d-in-simulant)
2. [Setting Up a 2D Camera](#2-setting-up-a-2d-camera)
3. [Sprites and Textured Quads](#3-sprites-and-textured-quads)
4. [Texture Atlases](#4-texture-atlases)
5. [2D Physics](#5-2d-physics)
6. [Tile Maps](#6-tile-maps)
7. [2D Animations](#7-2d-animations)
8. [UI as Gameplay](#8-ui-as-gameplay)
9. [Camera Effects](#9-camera-effects)
10. [Performance Tips for 2D](#10-performance-tips-for-2d)
11. [Platform-Specific Considerations](#11-platform-specific-considerations)

---

## 1. Overview: 2D in Simulant

Simulant is fundamentally a 3D engine, but it provides excellent support for 2D games through:

- **Camera2D** -- an orthographic camera that renders a flat coordinate space
- **Sprites** -- textured billboard quads for 2D graphics
- **Geom** with **Quadtree** culling -- efficient rendering for large static 2D worlds
- **UI System** -- can serve as a lightweight 2D rendering layer
- **Physics** -- works equally well in 2D by constraining movement to a plane

### 2D Coordinate System

In Simulant's 2D space:
- **+X** points right
- **+Y** points up
- The origin (0, 0) is at the center of the camera's view by default

For UI rendering, coordinates are typically in **screen space** (pixels from the top-left or bottom-left depending on camera setup).

---

## 2. Setting Up a 2D Camera

The `Camera2D` class provides an orthographic projection -- parallel projection lines with no perspective distortion. This is essential for 2D games where objects should appear the same size regardless of depth.

### Basic 2D Camera

```cpp
class MyScene : public smlt::Scene {
private:
    smlt::CameraPtr camera_;
    smlt::LayerPtr pipeline_;

    void on_load() override {
        // Create a 2D orthographic camera
        camera_ = create_child<smlt::Camera2D>();

        // Set the orthographic projection to match window dimensions
        // Arguments: left, right, bottom, top
        camera_->set_orthographic_projection(
            0,
            window->width(),
            0,
            window->height()
        );

        // Create the render pipeline
        pipeline_ = compositor->create_layer(this, camera_);
        pipeline_->set_clear_flags(smlt::BUFFER_CLEAR_ALL);
        pipeline_->viewport->set_color(smlt::Color(0.0f, 0.0f, 0.0f, 1.0f));
    }
};
```

### World-Space 2D Camera

For games that use world coordinates rather than pixel coordinates:

```cpp
// A 20-unit wide view, centered on the player
float view_width = 20.0f;
float view_height = view_width / window->aspect_ratio();

camera_->set_orthographic_projection(
    -view_width / 2.0f,
    view_width / 2.0f,
    -view_height / 2.0f,
    view_height / 2.0f
);
```

### Scrolling Camera

For games with a camera that follows the player:

```cpp
void on_update(float dt) override {
    Scene::on_update(dt);

    // Move the camera to follow the player
    if (player_) {
        auto pos = player_->absolute_position();

        // For Camera2D, adjust the orthographic projection center
        float half_width = window->width() / 2.0f;
        float half_height = window->height() / 2.0f;

        camera_->set_orthographic_projection(
            pos.x - half_width,
            pos.x + half_width,
            pos.y - half_height,
            pos.y + half_height
        );
    }
}
```

### Smooth Camera Following

For a camera that eases toward the player rather than snapping:

```cpp
smlt::Vec2 camera_target_ = {0, 0};
smlt::Vec2 camera_current_ = {0, 0};
float camera_smoothness_ = 5.0f;  // Higher = snappier

void on_update(float dt) override {
    Scene::on_update(dt);

    if (player_) {
        camera_target_ = player_->absolute_position().xy();
    }

    // Smooth interpolation
    float t = 1.0f - std::exp(-camera_smoothness_ * dt);
    camera_current_ = camera_current_.lerp(camera_target_, t);

    float half_width = window->width() / 2.0f;
    float half_height = window->height() / 2.0f;

    camera_->set_orthographic_projection(
        camera_current_.x - half_width,
        camera_current_.x + half_width,
        camera_current_.y - half_height,
        camera_current_.y + half_height
    );
}
```

---

## 3. Sprites and Textured Quads

### Creating a Sprite

Sprites in Simulant are `Actor` nodes with a single textured quad. The simplest approach:

```cpp
// Create a sprite from a texture
auto texture = assets->load_texture("textures/hero.png");
auto material = assets->create_material_from_texture(texture);

auto sprite = create_child<smlt::Actor>();
auto mesh = assets->new_mesh_from_procedural_quad();
sprite->set_mesh(mesh->id());
sprite->set_material(material->id());
sprite->set_scale(1.0f, 1.0f, 1.0f);  // Size in world units
sprite->transform->set_position_2d(smlt::Vec2(100, 100));
```

### Sprite Flipping

Flip sprites horizontally or vertically for directional facing:

```cpp
// Flip horizontally (for left-facing character)
sprite->transform->set_scale(-1.0f, 1.0f, 1.0f);

// Flip vertically
sprite->transform->set_scale(1.0f, -1.0f, 1.0f);

// Reset
sprite->transform->set_scale(1.0f, 1.0f, 1.0f);
```

### Sprite Rotation

```cpp
// Rotate in 2D (around Z axis)
sprite->transform->set_rotation(
    smlt::Quaternion::angle_axis(smlt::Degrees(45.0f), smlt::Vec3(0, 0, 1))
);
```

### Sprite Sheets

For animation, use a sprite sheet (a single texture containing multiple frames):

```cpp
auto sprite_sheet = assets->load_texture("textures/hero_spritesheet.png");

// Adjust UV coordinates to show a specific frame
float frame_width = 1.0f / 4.0f;  // 4 frames horizontally
float frame_u = frame_index * frame_width;

smlt::Mat3 uv_transform = smlt::Mat3::identity();
// Set translation and scale for the UV transform
material->set_property_value("s_diffuse_map_matrix", uv_transform);
```

---

## 4. Texture Atlases

A texture atlas combines many small images into a single large texture. This is one of the most important optimizations for 2D games.

### Why Use Atlases

Without an atlas, each sprite with a unique texture triggers a separate draw call. With an atlas, all sprites sharing the atlas can be batched into a single draw call.

### Creating an Atlas

Organize your textures into a single PNG:

```
+--------+--------+--------+--------+
|  tile1 |  tile2 |  tile3 |  tile4 |
+--------+--------+--------+--------+
| enemy1 | enemy2 |  item1 |  item2 |
+--------+--------+--------+--------+
|  player|  effect|   ui1  |   ui2  |
+--------+--------+--------+--------+
```

Each tile should be the same size (e.g., 64x64) for easy indexing:

```cpp
auto atlas = assets->load_texture("textures/atlas.png");
auto atlas_material = assets->create_material_from_texture(atlas);
atlas_material->set_garbage_collection_method(smlt::GARBAGE_COLLECT_NEVER);

// Calculate UV coordinates for a specific tile
int tile_x = 2;  // Column
int tile_y = 1;  // Row
int tiles_per_row = 4;
float tile_u = 1.0f / tiles_per_row;
float tile_v = 1.0f / 3.0f;  // 3 rows

// UV offset for this tile
float u_offset = tile_x * tile_u;
float v_offset = tile_y * tile_v;

// Apply UV transform to the material
smlt::Mat3 uv_matrix;
uv_matrix.set_translation(u_offset, v_offset);
uv_matrix.set_scale(tile_u, tile_v);
atlas_material->set_property_value("s_diffuse_map_matrix", uv_matrix);
```

### Atlas Size Limits

| Platform | Maximum Atlas Size |
|----------|-------------------|
| Desktop | GPU-dependent (typically 4096-8192) |
| Dreamcast | 1024 pixels |
| PSP | **512 pixels** (power-of-two required) |

On constrained platforms, you may need multiple smaller atlases rather than one large one.

---

## 5. 2D Physics

Simulant's physics system works well for 2D games. The key is to constrain movement to a plane.

### Setting Up 2D Physics

```cpp
void on_load() override {
    // Create physics scene with gravity pointing down
    physics_scene_ = create_child<smlt::PhysicsScene>();
    physics_scene_->set_gravity(smlt::Vec3(0, -20.0f, 0));
}
```

### Constraining to 2D

For a side-scroller, lock the Z axis:

```cpp
auto player_body = player_->create_mixin<smlt::DynamicBody>();
player_body->add_box_collider(
    smlt::Vec3(0.8f, 1.6f, 0.1f),  // Thin box (Z is minimal)
    smlt::PhysicsMaterial::wood()
);
player_body->set_mass(1.0f);

// Lock rotation and Z movement by zeroing them each frame
void on_fixed_update(float step) override {
    if (player_body_) {
        auto vel = player_body_->linear_velocity();
        player_body_->set_linear_velocity(smlt::Vec3(vel.x, vel.y, 0));

        auto ang_vel = player_body_->angular_velocity();
        player_body_->set_angular_velocity(smlt::Vec3(0, 0, ang_vel.z));
    }
}
```

### Top-Down Physics

For a top-down game, use no gravity and control all movement directly:

```cpp
physics_scene_ = create_child<smlt::PhysicsScene>();
physics_scene_->set_gravity(smlt::Vec3(0, 0, 0));  // No gravity

void handle_input(float dt) {
    if (!player_body_) return;

    smlt::Vec2 input(0, 0);
    if (input_->key_pressed(smlt::KEY_W)) input.y += 1;
    if (input_->key_pressed(smlt::KEY_S)) input.y -= 1;
    if (input_->key_pressed(smlt::KEY_A)) input.x -= 1;
    if (input_->key_pressed(smlt::KEY_D)) input.x += 1;

    if (input.length() > 0) {
        input = input.normalized() * speed_;
        player_body_->set_linear_velocity(
            smlt::Vec3(input.x, input.y, 0)
        );
    }
}
```

### 2D Collision Detection

Use collision signals for game events:

```cpp
player_body_->signal_contact_begin().connect(
    [this](smlt::FixturePtr a, smlt::FixturePtr b) {
        // Determine what was hit
        auto other = (a->body() == player_body_) ? b : a;

        if (other->body()->node()->name() == "Coin") {
            collect_coin(other->body()->node());
        }
        if (other->body()->node()->name() == "Hazard") {
            take_damage();
        }
    }
);
```

### Sensors (Trigger Zones)

Use sensor fixtures for non-solid collision detection:

```cpp
// Create a sensor trigger zone
auto sensor_body = trigger_zone->create_mixin<smlt::StaticBody>();
auto fixture = sensor_body->add_box_collider(
    smlt::Vec3(2.0f, 2.0f, 0.1f),
    smlt::PhysicsMaterial::default_material()
);
fixture->set_is_sensor(true);  // Non-solid, just detects overlap

sensor_body->signal_contact_begin().connect(
    [this](smlt::FixturePtr a, smlt::FixturePtr b) {
        // Player entered trigger zone
        trigger_activated();
    }
);
```

---

## 6. Tile Maps

### Loading Tiled TMX Maps

Simulant supports the Tiled TMX format for 2D tile maps:

```cpp
auto tilemap = assets->load_tilemap("maps/level_1.tmx");
```

### Manual Tile Rendering

For custom tile systems, use `Geom` for the static tile layer:

```cpp
void build_tilemap() {
    int map_width = 100;
    int map_height = 50;
    int tile_size = 32;

    // Create a mesh for the entire tilemap
    auto mesh = assets->new_mesh();

    for (int y = 0; y < map_height; ++y) {
        for (int x = 0; x < map_width; ++x) {
            int tile_id = get_tile_at(x, y);
            if (tile_id == 0) continue;  // Empty tile

            float px = x * tile_size;
            float py = y * tile_size;

            // Calculate UVs from atlas
            float u = (tile_id % atlas_cols) / (float) atlas_cols;
            float v = (tile_id / atlas_cols) / (float) atlas_rows;
            float u_tile = 1.0f / atlas_cols;
            float v_tile = 1.0f / atlas_rows;

            // Add quad for this tile
            mesh->new_quad(
                smlt::Vec3(px, py, 0),
                smlt::Vec3(px + tile_size, py + tile_size, 0),
                smlt::Vec2(u, v),
                smlt::Vec2(u + u_tile, v + v_tile)
            );
        }
    }

    mesh->upload_to_vram();

    // Use Geom for static tilemap (efficient culling)
    auto tilemap_geom = create_child<smlt::Geom>(mesh->id(), smlt::Vec3::zero());
}
```

### Tile-Based Collision

For tile-based collision, you can create physics colliders from tile data:

```cpp
void build_tile_collision() {
    // Group contiguous solid tiles into rectangular colliders
    // This reduces the number of physics bodies needed

    auto ground_body = ground_->create_mixin<smlt::StaticBody>();

    for (int y = 0; y < map_height; ++y) {
        for (int x = 0; x < map_width; ++x) {
            if (is_solid_tile(x, y)) {
                // Create a box collider for this tile
                ground_body->add_box_collider(
                    smlt::Vec3(tile_size, tile_size, 0.1f),
                    smlt::PhysicsMaterial::stone()
                );
                // Position is handled by the parent node transform
            }
        }
    }
}
```

---

## 7. 2D Animations

### Frame-Based Animation

For sprite-sheet animation, cycle through frames:

```cpp
class AnimatedSprite {
public:
    void update(float dt) {
        frame_timer_ += dt;
        if (frame_timer_ >= frame_duration_) {
            frame_timer_ = 0;
            current_frame_ = (current_frame_ + 1) % frame_count_;
            update_uv();
        }
    }

private:
    void update_uv() {
        float u = current_frame_ * frame_width_;
        smlt::Mat3 uv_matrix;
        uv_matrix.set_translation(u, 0);
        uv_matrix.set_scale(frame_width_, 1.0f);
        material_->set_property_value("s_diffuse_map_matrix", uv_matrix);
    }

    float frame_timer_ = 0;
    float frame_duration_ = 0.1f;  // 10 FPS animation
    int current_frame_ = 0;
    int frame_count_ = 4;
    float frame_width_ = 0.25f;  // 4 frames in atlas
    smlt::MaterialPtr material_;
};
```

### Using Coroutines for Animation

```cpp
void play_animation(int frames, float duration_per_frame) {
    cr_async([this, frames, duration_per_frame]() {
        for (int i = 0; i < frames; ++i) {
            current_frame_ = i;
            update_uv();
            cr_yield_for(smlt::Seconds(duration_per_frame));
        }
    });
}
```

---

## 8. UI as Gameplay

Simulant's UI system can serve as a lightweight 2D rendering layer for simple games or game overlays.

### UI-Based 2D Game

For very simple 2D games (puzzle games, card games, menus), the UI system alone may be sufficient:

```cpp
void create_game_board() {
    auto board = create_child<smlt::ui::Frame>("");
    board->set_layout_direction(smlt::ui::LAYOUT_DIRECTION_HORIZONTAL);
    board->set_space_between(5);

    for (int i = 0; i < 8; ++i) {
        auto cell = create_child<smlt::ui::Widget>("");
        cell->resize(64, 64);
        cell->signal_clicked().connect([this, i]() {
            cell_tapped(i);
        });
        board->pack_child(cell);
    }
}
```

### HUD Overlay

Render a HUD on top of your 2D game using a separate camera:

```cpp
void on_load() override {
    // Main game camera
    game_camera_ = create_child<smlt::Camera2D>();
    game_camera_->set_orthographic_projection(
        0, window->width(), 0, window->height()
    );
    game_pipeline_ = compositor->create_layer(this, game_camera_);

    // HUD camera (separate layer rendered on top)
    hud_camera_ = create_child<smlt::Camera2D>();
    hud_camera_->set_orthographic_projection(
        0, window->width(), 0, window->height()
    );
    hud_pipeline_ = compositor->create_layer(this, hud_camera_);
    hud_pipeline_->set_clear_flags(smlt::BUFFER_CLEAR_NONE);  // Don't clear -- overlay
}

void create_hud() {
    // Score label (top-left)
    auto score_label = create_child<smlt::ui::Label>("Score: 0");
    score_label->transform->set_position_2d(smlt::Vec2(20, window->height() - 40));
    score_label->set_font_size(24);

    // Health bar (top-right)
    auto health_bar = create_child<smlt::ui::ProgressBar>("");
    health_bar->resize(200, 20);
    health_bar->transform->set_position_2d(
        smlt::Vec2(window->width() - 220, window->height() - 40)
    );
    health_bar->set_value(1.0f);  // Full health
}
```

---

## 9. Camera Effects

### Screen Shake

```cpp
float shake_intensity_ = 0;
float shake_duration_ = 0;
float shake_timer_ = 0;

void start_shake(float intensity, float duration) {
    shake_intensity_ = intensity;
    shake_duration_ = duration;
    shake_timer_ = 0;
}

void on_update(float dt) override {
    Scene::on_update(dt);

    if (shake_timer_ < shake_duration_) {
        shake_timer_ += dt;
        float t = 1.0f - (shake_timer_ / shake_duration_);  // Decay
        float shake = shake_intensity_ * t;

        float offset_x = (rand_float(-1, 1)) * shake;
        float offset_y = (rand_float(-1, 1)) * shake;

        // Apply offset to camera projection
        camera_->set_orthographic_projection(
            base_left_ + offset_x,
            base_right_ + offset_x,
            base_bottom_ + offset_y,
            base_top_ + offset_y
        );
    }
}
```

### Zoom

```cpp
void set_zoom(float zoom_level) {
    float half_w = (window->width() / 2.0f) / zoom_level;
    float half_h = (window->height() / 2.0f) / zoom_level;

    smlt::Vec2 center = camera_current_;
    camera_->set_orthographic_projection(
        center.x - half_w,
        center.x + half_w,
        center.y - half_h,
        center.y + half_h
    );
}
```

---

## 10. Performance Tips for 2D

### Use Geom for Static Content

Background tiles, decorative elements, and level geometry that never moves should use `Geom` instead of `Actor`:

```cpp
// Static background -- use Geom
auto bg_geom = create_child<smlt::Geom>(bg_mesh_id, smlt::Vec3::zero());

// Moving character -- use Actor
auto player = create_child<smlt::Actor>(player_mesh_id);
```

See [Geom vs Actor](../rendering/geom.md) for full details.

### Batch with Shared Materials

All sprites using the same texture atlas should share a single material:

```cpp
auto atlas = assets->load_texture("textures/atlas.png");
auto shared_material = assets->create_material_from_texture(atlas);
shared_material->set_garbage_collection_method(smlt::GARBAGE_COLLECT_NEVER);

// All sprites use this material with different UV transforms
for (int i = 0; i < 100; ++i) {
    auto sprite = create_child<smlt::Actor>(quad_mesh_id);
    sprite->set_material(shared_material->id());
    // Set UV transform per-sprite via material property
}
```

### Minimize Texture Swaps

Preloading all textures at scene start avoids loading hitches during gameplay:

```cpp
void on_load() override {
    // Preload all textures
    for (const auto& path : texture_paths_) {
        assets->load_texture(path, smlt::GARBAGE_COLLECT_NEVER);
    }
}
```

### Object Pooling for Bullets and Particles

```cpp
class BulletPool {
public:
    void initialize(size_t count, smlt::Scene* scene, MeshID mesh_id, MaterialID mat_id) {
        scene_ = scene;
        mesh_id_ = mesh_id;
        mat_id_ = mat_id;
        for (size_t i = 0; i < count; ++i) {
            auto bullet = scene_->create_child<smlt::Actor>(mesh_id_);
            bullet->set_material(mat_id_);
            bullet->set_visible(false);
            bullet->set_enabled(false);
            pool_.push_back(bullet);
        }
    }

    smlt::ActorPtr acquire() {
        for (auto& bullet : pool_) {
            if (!bullet->is_enabled()) {
                bullet->set_visible(true);
                bullet->set_enabled(true);
                return bullet;
            }
        }
        return nullptr;
    }

    void release(smlt::ActorPtr bullet) {
        bullet->set_visible(false);
        bullet->set_enabled(false);
    }

private:
    smlt::Scene* scene_;
    MeshID mesh_id_;
    MaterialID mat_id_;
    std::vector<smlt::ActorPtr> pool_;
};
```

---

## 11. Platform-Specific Considerations

### Dreamcast

| Constraint | Value | Guidance |
|-----------|-------|----------|
| RAM | 16MB | Keep texture atlases small; embed fonts |
| Max texture size | 1024px | Use power-of-two dimensions |
| Renderer | GL 1.x (fixed-function) | No custom shaders |
| Audio | OGG/Vorbis (streamed), WAV | Stream music, load SFX into memory |
| Asset loading | Embedded in ELF | All assets built into executable |

```cpp
#ifdef SIMULANT_PLATFORM_DREAMCAST
    // Use smaller atlas for Dreamcast
    auto atlas = assets->load_texture("textures/atlas_512.png");
#else
    auto atlas = assets->load_texture("textures/atlas_1024.png");
#endif
```

### PSP

| Constraint | Value | Guidance |
|-----------|-------|----------|
| RAM | 32MB | More headroom than Dreamcast |
| Max texture size | **512px** | **Must be power-of-two** |
| Max lights | 4 per object | Keep lighting simple |
| Audio | WAV only | OGG not supported |
| Asset loading | Embedded | All assets built into executable |

```cpp
#ifdef SIMULANT_PLATFORM_PSP
    // All textures must be power-of-two, max 512px
    // Use WAV audio instead of OGG
    auto music = assets->load_sound("music/level.wav");
#else
    auto music = assets->load_sound("music/level.ogg");
#endif
```

### Desktop

Desktop platforms have far fewer constraints. Focus on:
- Using DDS textures with DXT compression for VRAM efficiency
- Loading assets asynchronously to avoid hitches
- Using LOD for large numbers of sprites

---

## Summary

Key takeaways for 2D game development in Simulant:

1. **Use Camera2D** with orthographic projection for flat rendering
2. **Use texture atlases** to batch draw calls -- one atlas, one material, many sprites
3. **Use Geom** for static 2D geometry (tilemaps, backgrounds)
4. **Use Actor** for moving objects (player, enemies, projectiles)
5. **Pool frequently created objects** (bullets, particles, pickups)
6. **Constrain physics to 2D** by zeroing Z-axis movement
7. **Preload all assets** at scene start to avoid runtime hitches
8. **Use platform-specific asset variants** for constrained platforms

---

## Further Reading

- [Asset Pipeline Guide](asset-pipeline.md) -- Preparing and loading assets
- [Performance Guide](performance.md) -- Optimization techniques
- [Sprites](../rendering/sprites.md) -- Detailed sprite documentation
- [UI System](../ui/overview.md) -- UI widgets and layout
- [Physics Overview](../physics/overview.md) -- Physics system details
- [Building a Complete Game](complete-game.md) -- Step-by-step game tutorial
