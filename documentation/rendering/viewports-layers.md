# Viewports and Layers

This guide covers viewports and the Layer system in detail. Layers (also called render pipelines) are the core rendering primitive in Simulant -- they connect a camera, a scene subtree, and a rectangular viewport into a single renderable unit. Understanding how to configure and combine layers is essential for building anything beyond the simplest single-camera game.

**See also:** [Render Pipelines and Compositor](pipelines.md), [Cameras](../core-concepts/cameras.md), [Scenes](../scene.md), [Stage Nodes](../stage_nodes.md)

---

## Table of Contents

1. [What Is a Layer?](#1-what-is-a-layer)
2. [The Viewport Class](#2-the-viewport-class)
3. [Built-in Viewport Types](#3-built-in-viewport-types)
4. [Custom Viewports](#4-custom-viewports)
5. [Split-Screen Rendering](#5-split-screen-rendering)
6. [Layered UI Overlays](#6-layered-ui-overlays)
7. [Render-to-Texture with Custom Viewports](#7-render-to-texture-with-custom-viewports)
8. [Layer Priority and Viewport Interaction](#8-layer-priority-and-viewport-interaction)
9. [Minimap Example](#9-minimap-example)
10. [Picture-in-Picture Rear-View Mirror](#10-picture-in-picture-rear-view-mirror)
11. [Best Practices](#11-best-practices)

---

## 1. What Is a Layer?

A `Layer` (or render pipeline) is a rendering unit owned by the `Compositor`. Each layer bundles together:

- A **camera** -- determines the viewpoint and projection
- A **stage node subtree** -- determines what is rendered (can be the entire scene or a single actor)
- A **viewport** -- determines where on the render target the layer draws
- A **target texture** -- determines which framebuffer or off-screen texture is drawn to
- A **priority** -- determines the order layers are rendered in
- **Clear flags** -- determines which buffers are cleared before rendering

Layers are created via `Compositor::create_layer()` and return a `LayerPtr`:

```cpp
LayerPtr create_layer(
    StageNode* subtree,          // The stage subtree to render
    CameraPtr camera,             // The camera to view it with
    const Viewport& viewport,     // Viewport region (default = full screen)
    TexturePtr target,            // Render target texture (default = screen)
    int32_t priority = 0          // Render priority (default = 0)
);
```

Every layer starts **deactivated**. You must call `activate()` for it to produce output:

```cpp
auto layer = compositor->create_layer(this, camera_);
layer->activate();
```

Inside a `Scene`, the `compositor` convenience member is a `SceneCompositor` that automatically tracks and cleans up layers tied to the scene's lifecycle. Always prefer it over `window->compositor`:

```cpp
// GOOD -- inside a Scene
auto layer = compositor->create_layer(this, camera_);

// AVOID -- creating layers on the global compositor from within a scene
auto layer = window->compositor->create_layer(this, camera_);
```

---

## 2. The Viewport Class

A `Viewport` defines the rectangular region within a render target that a layer draws into. It stores its bounds as **ratios** -- normalized values between 0.0 and 1.0 that scale automatically to the render target's pixel dimensions.

### Definition

```cpp
namespace smlt {

typedef float Ratio; // Value between 0.0 and 1.0

class Viewport {
public:
    Viewport();
    Viewport(ViewportType type, const Color& color = Color::black());
    Viewport(Ratio x, Ratio y, Ratio width, Ratio height,
             const Color& color = Color::black());

    Ratio x() const;
    Ratio y() const;
    Ratio width() const;
    Ratio height() const;

    uint32_t width_in_pixels(const RenderTarget& target) const;
    uint32_t height_in_pixels(const RenderTarget& target) const;

    ViewportType type() const;

    void set_color(const Color& color);
    const Color& color() const;
};

}
```

### The Ratio Type

`Ratio` is a `typedef` for `float`, conventionally restricted to the range 0.0 to 1.0. A ratio of `0.5` means "halfway across the target." Ratios automatically adapt when the window or render target is resized -- you do not need to recalculate viewport bounds on resize.

### Viewport Constructors

**Constructor 1: Type-based**

The most common way to create a viewport. Pass a `ViewportType` enum value and an optional clear color:

```cpp
// Full-screen viewport with black clear color
smlt::Viewport vp(smlt::VIEWPORT_TYPE_FULL, smlt::Color::black());

// Left half of the screen with dark blue clear
smlt::Viewport vp(smlt::VIEWPORT_TYPE_VERTICAL_SPLIT_LEFT, smlt::Color(0.0f, 0.0f, 0.2f, 1.0f));
```

**Constructor 2: Custom ratios**

For arbitrary positioning and sizing, specify x, y, width, and height as ratios:

```cpp
// A small viewport in the bottom-left corner (25% width, 25% height)
smlt::Viewport minimap_vp(
    smlt::Ratio(0.0f),   // x: left edge of target
    smlt::Ratio(0.7f),   // y: 70% from top (bottom 30% area)
    smlt::Ratio(0.25f),  // width: 25% of target width
    smlt::Ratio(0.25f),  // height: 25% of target height
    smlt::Color::black()
);
```

### Pixel Dimensions

To find the actual pixel size of a viewport for a given render target:

```cpp
uint32_t pixel_w = viewport.width_in_pixels(*render_target);
uint32_t pixel_h = viewport.height_in_pixels(*render_target);
```

For the default framebuffer, the render target is the window's back buffer.

---

## 3. Built-in Viewport Types

Simulant provides several predefined viewport layouts in the `ViewportType` enum:

| Viewport Type | Description | Region |
|---------------|-------------|--------|
| `VIEWPORT_TYPE_FULL` | Fills the entire target | 0, 0, 1.0, 1.0 |
| `VIEWPORT_TYPE_VERTICAL_SPLIT_LEFT` | Left half of the screen | 0, 0, 0.5, 1.0 |
| `VIEWPORT_TYPE_VERTICAL_SPLIT_RIGHT` | Right half of the screen | 0.5, 0, 0.5, 1.0 |
| `VIEWPORT_TYPE_HORIZONTAL_SPLIT_TOP` | Top half of the screen | 0, 0.5, 1.0, 0.5 |
| `VIEWPORT_TYPE_HORIZONTAL_SPLIT_BOTTOM` | Bottom half of the screen | 0, 0, 1.0, 0.5 |
| `VIEWPORT_TYPE_BLACKBAR_4_BY_3` | 4:3 letterboxed on wider screens | Centered 4:3 region |
| `VIEWPORT_TYPE_BLACKBAR_16_BY_9` | 16:9 letterboxed | Centered 16:9 region |
| `VIEWPORT_TYPE_BLACKBAR_16_BY_10` | 16:10 letterboxed | Centered 16:10 region |
| `VIEWPORT_TYPE_CUSTOM` | Use ratio constructor | You define the region |

### Letterboxing

The `BLACKBAR` viewport types automatically calculate the centered region that fits the specified aspect ratio within the render target. Pixels outside the region are filled with the viewport's clear color. This is useful for maintaining a consistent aspect ratio regardless of the actual window size:

```cpp
// 4:3 letterboxed -- black bars appear on left and right on wide screens
smlt::Viewport vp(smlt::VIEWPORT_TYPE_BLACKBAR_4_BY_3, smlt::Color::black());

layer->set_viewport(vp);
```

---

## 4. Custom Viewports

For any layout not covered by the built-in types, use `VIEWPORT_TYPE_CUSTOM` (implicitly selected when you use the ratio constructor).

### Positioning and Sizing

Ratios are relative to the render target:

- `(0.0, 0.0)` is the top-left corner
- `(1.0, 1.0)` is the bottom-right corner
- `width` and `height` are fractions of the target's width and height

### Example: Heads-Up Display Region

A narrow strip along the bottom of the screen for a HUD:

```cpp
smlt::Viewport hud_vp(
    smlt::Ratio(0.0f),   // x: full width
    smlt::Ratio(0.85f),  // y: bottom 15% of screen
    smlt::Ratio(1.0f),   // width: full width
    smlt::Ratio(0.15f),  // height: 15% of screen height
    smlt::Color(0.0f, 0.0f, 0.0f, 1.0f)
);

auto hud_layer = compositor->create_layer(ui_stage, ui_camera)
    ->set_viewport(hud_vp)
    ->set_clear_flags(smlt::BUFFER_CLEAR_ALL)
    ->set_priority(smlt::RENDER_PRIORITY_FOREGROUND);

hud_layer->activate();
```

### Example: Side Panel

A vertical panel occupying the right 20% of the screen:

```cpp
smlt::Viewport side_panel(
    smlt::Ratio(0.8f),   // x: starts at 80% across
    smlt::Ratio(0.0f),   // y: top of screen
    smlt::Ratio(0.2f),   // width: 20% of screen
    smlt::Ratio(1.0f),   // height: full height
    smlt::Color(0.1f, 0.1f, 0.15f, 1.0f)
);
```

### Overlapping Viewports

Multiple layers can draw into overlapping viewport regions. The render order is determined by layer priority, not viewport position:

```cpp
// Background: full-screen 3D scene
auto main_layer = compositor->create_layer(this, camera3d_)
    ->set_viewport(smlt::Viewport(smlt::VIEWPORT_TYPE_FULL, smlt::Color::blue()))
    ->set_clear_flags(smlt::BUFFER_CLEAR_ALL)
    ->set_priority(0);

// Foreground: small inset viewport drawn on top
auto inset_layer = compositor->create_layer(this, inset_camera_)
    ->set_viewport(smlt::Viewport(
        smlt::Ratio(0.7f), smlt::Ratio(0.7f),
        smlt::Ratio(0.25f), smlt::Ratio(0.25f),
        smlt::Color::black()
    ))
    ->set_clear_flags(smlt::BUFFER_CLEAR_ALL)
    ->set_priority(1);
```

Because `inset_layer` has a higher priority, it renders after `main_layer` and appears on top, even though their viewport regions overlap.

---

## 5. Split-Screen Rendering

Split-screen multiplayer is one of the most common uses of multiple viewports. Each player gets their own camera and their own half of the screen.

### Two-Player Vertical Split

```cpp
class SplitScreenScene : public smlt::Scene {
public:
    SplitScreenScene(smlt::Window* window) : smlt::Scene(window) {}

    void on_load() override {
        // Two cameras
        auto camera1 = create_child<smlt::Camera3D>();
        auto camera2 = create_child<smlt::Camera3D>();

        camera1->transform->set_position(-5, 5, -10);
        camera1->transform->look_at(0, 0, 0);

        camera2->transform->set_position(5, 5, -10);
        camera2->transform->look_at(0, 0, 0);

        // Left viewport for Player 1
        smlt::Viewport left_vp(
            smlt::VIEWPORT_TYPE_VERTICAL_SPLIT_LEFT,
            smlt::Color(0.15f, 0.0f, 0.0f, 1.0f)  // Dark red tint
        );

        // Right viewport for Player 2
        smlt::Viewport right_vp(
            smlt::VIEWPORT_TYPE_VERTICAL_SPLIT_RIGHT,
            smlt::Color(0.0f, 0.0f, 0.15f, 1.0f)  // Dark blue tint
        );

        compositor->create_layer(this, camera1)
            ->set_viewport(left_vp)
            ->set_clear_flags(smlt::BUFFER_CLEAR_ALL)
            ->set_priority(0)
            ->set_name("player1");

        compositor->create_layer(this, camera2)
            ->set_viewport(right_vp)
            ->set_clear_flags(smlt::BUFFER_CLEAR_ALL)
            ->set_priority(0)
            ->set_name("player2");

        // Add shared geometry
        auto mesh = assets->create_mesh(smlt::VertexSpecification::DEFAULT);
        mesh->create_submesh_as_cube("cube", assets->create_material(), 2.0f);
        create_child<smlt::Actor>(mesh);
    }
};
```

### Four-Player Split

For four players, combine horizontal and vertical splits:

```cpp
void on_load() override {
    auto cameras = std::vector<smlt::CameraPtr>(4);
    auto viewports = std::vector<smlt::Viewport>(4);

    for (int i = 0; i < 4; ++i) {
        cameras[i] = create_child<smlt::Camera3D>();
        cameras[i]->transform->set_position(
            (i % 2 == 0 ? -5.0f : 5.0f),
            5.0f,
            (i < 2 ? -10.0f : 5.0f)
        );
    }

    // Top-left
    viewports[0] = smlt::Viewport(
        smlt::Ratio(0.0f), smlt::Ratio(0.0f),
        smlt::Ratio(0.5f), smlt::Ratio(0.5f),
        smlt::Color(0.1f, 0.0f, 0.0f, 1.0f)
    );

    // Top-right
    viewports[1] = smlt::Viewport(
        smlt::Ratio(0.5f), smlt::Ratio(0.0f),
        smlt::Ratio(0.5f), smlt::Ratio(0.5f),
        smlt::Color(0.0f, 0.1f, 0.0f, 1.0f)
    );

    // Bottom-left
    viewports[2] = smlt::Viewport(
        smlt::Ratio(0.0f), smlt::Ratio(0.5f),
        smlt::Ratio(0.5f), smlt::Ratio(0.5f),
        smlt::Color(0.0f, 0.0f, 0.1f, 1.0f)
    );

    // Bottom-right
    viewports[3] = smlt::Viewport(
        smlt::Ratio(0.5f), smlt::Ratio(0.5f),
        smlt::Ratio(0.5f), smlt::Ratio(0.5f),
        smlt::Color(0.1f, 0.1f, 0.0f, 1.0f)
    );

    for (int i = 0; i < 4; ++i) {
        compositor->create_layer(this, cameras[i])
            ->set_viewport(viewports[i])
            ->set_clear_flags(smlt::BUFFER_CLEAR_ALL)
            ->set_name("player" + std::to_string(i + 1));
    }
}
```

---

## 6. Layered UI Overlays

A common pattern is a full-screen 3D scene with a 2D UI overlay drawn on top. The key is to use **different clear flags** so the UI layer does not erase what the 3D layer already drew.

```cpp
class GameScene : public smlt::Scene {
public:
    GameScene(smlt::Window* window) : smlt::Scene(window) {}

    void on_load() override {
        // ---- 3D World ----
        camera3d_ = create_child<smlt::Camera3D>();
        camera3d_->set_perspective_projection(
            smlt::Degrees(45.0f),
            float(window->width()) / float(window->height()),
            0.1f, 1000.0f
        );

        pipeline3d_ = compositor->create_layer(this, camera3d_)
            ->set_clear_flags(smlt::BUFFER_CLEAR_ALL)  // Clear color + depth
            ->set_priority(smlt::RENDER_PRIORITY_MAIN)
            ->set_name("main_3d");

        // ---- 2D UI Overlay ----
        camera2d_ = create_child<smlt::Camera2D>();
        camera2d_->set_orthographic_projection(
            0, window->width(), 0, window->height()
        );

        pipeline_ui_ = compositor->create_layer(this, camera2d_)
            ->set_clear_flags(0)  // Do NOT clear -- draw on top
            ->set_priority(smlt::RENDER_PRIORITY_FOREGROUND)
            ->set_name("ui_overlay");

        link_pipeline(pipeline3d_);
        link_pipeline(pipeline_ui_);
    }

private:
    smlt::CameraPtr camera3d_;
    smlt::CameraPtr camera2d_;
    smlt::LayerPtr pipeline3d_;
    smlt::LayerPtr pipeline_ui_;
};
```

**Key points:**
- The 3D layer clears both color and depth buffers (`BUFFER_CLEAR_ALL`)
- The UI layer clears **nothing** (`set_clear_flags(0)`) so it draws over the 3D scene
- The UI layer has higher priority (10 > 0), so it renders after the 3D layer

---

## 7. Render-to-Texture with Custom Viewports

When rendering to a texture, the viewport ratios are relative to the **texture dimensions**, not the screen. This is important when your render target is a different size than the window.

```cpp
void on_load() override {
    // Create a 512x512 off-screen texture
    auto rt_texture = app->shared_assets->create_texture(512, 512);

    // Camera for the off-screen view
    auto rt_camera = create_child<smlt::Camera3D>();
    rt_camera->transform->set_position(0, 10, -15);
    rt_camera->transform->look_at(0, 0, 0);

    // Full viewport of the texture (default if not specified)
    auto rt_layer = compositor->create_layer(this, rt_camera)
        ->set_target(rt_texture)
        ->set_clear_flags(smlt::BUFFER_CLEAR_ALL)
        ->set_name("offscreen_render");

    rt_layer->activate();

    // Use rt_texture as a material on an in-world screen
    auto screen_mat = assets->create_material();
    screen_mat->set_base_color_map(rt_texture);
    screen_mat->set_lighting_enabled(false);  // Self-illuminated

    auto screen_mesh = assets->create_mesh(smlt::VertexSpecification::DEFAULT);
    screen_mesh->create_submesh_as_plane("screen", screen_mat, 4.0f, 3.0f);
    auto screen = create_child<smlt::Actor>(screen_mesh);
    screen->transform->set_position(5, 3, 0);
}
```

---

## 8. Layer Priority and Viewport Interaction

Layer priority determines render order; viewports determine *where* on the target each layer draws. These two properties are independent but work together to produce the final image.

### Render Order

Layers are sorted by ascending priority (lowest first). Within the same priority, creation order is used:

```cpp
// Rendered first (background)
auto bg_layer = compositor->create_layer(sky_stage, sky_camera)
    ->set_priority(smlt::RENDER_PRIORITY_BACKGROUND);

// Rendered second (main scene)
auto main_layer = compositor->create_layer(this, camera_)
    ->set_priority(smlt::RENDER_PRIORITY_MAIN);

// Rendered third (UI on top)
auto ui_layer = compositor->create_layer(ui_stage, ui_camera)
    ->set_priority(smlt::RENDER_PRIORITY_FOREGROUND);
```

### Viewport Independence

Each layer's viewport is independent of other layers. Two layers can have the same viewport, overlapping viewports, or completely separate viewports:

```cpp
// Both layers render to the full screen, but in sequence
auto layer_a = compositor->create_layer(this, camera_a)
    ->set_viewport(smlt::Viewport(smlt::VIEWPORT_TYPE_FULL))
    ->set_priority(0);

auto layer_b = compositor->create_layer(this, camera_b)
    ->set_viewport(smlt::Viewport(smlt::VIEWPORT_TYPE_FULL))
    ->set_priority(1);
// layer_b renders on top of layer_a (both fill the screen)
```

### Clear Flags and Overlapping Viewports

When viewports overlap, clear flags determine whether a layer erases what a previous layer drew in the overlapping region:

```cpp
// Layer A: full screen, clears everything
auto layer_a = compositor->create_layer(this, cam_a)
    ->set_viewport(smlt::Viewport(smlt::VIEWPORT_TYPE_FULL, smlt::Color::blue()))
    ->set_clear_flags(smlt::BUFFER_CLEAR_ALL)
    ->set_priority(0);

// Layer B: small inset viewport, also clears (draws its own content)
auto layer_b = compositor->create_layer(this, cam_b)
    ->set_viewport(smlt::Viewport(
        smlt::Ratio(0.7f), smlt::Ratio(0.7f),
        smlt::Ratio(0.25f), smlt::Ratio(0.25f),
        smlt::Color::black()
    ))
    ->set_clear_flags(smlt::BUFFER_CLEAR_ALL)
    ->set_priority(1);

// Layer C: UI overlay on the main area, does NOT clear
auto layer_c = compositor->create_layer(ui_stage, ui_cam)
    ->set_clear_flags(0)  // No clear -- preserve both layer A and B output
    ->set_priority(2);
```

---

## 9. Minimap Example

A top-down camera rendered to a small square viewport in the corner of the screen:

```cpp
class MinimapScene : public smlt::Scene {
public:
    MinimapScene(smlt::Window* window) : smlt::Scene(window) {}

    void on_load() override {
        // ---- Main 3D camera ----
        main_camera_ = create_child<smlt::Camera3D>();
        main_camera_->set_perspective_projection(
            smlt::Degrees(60.0f),
            float(window->width()) / float(window->height()),
            0.1f, 1000.0f
        );
        main_camera_->transform->set_position(0, 5, -20);

        auto main_layer = compositor->create_layer(this, main_camera_)
            ->set_clear_flags(smlt::BUFFER_CLEAR_ALL)
            ->set_priority(smlt::RENDER_PRIORITY_MAIN)
            ->set_name("main");

        // ---- Minimap camera (top-down orthographic) ----
        minimap_camera_ = create_child<smlt::Camera3D>();
        minimap_camera_->transform->set_position(0, 50, 0);
        minimap_camera_->transform->look_at(0, 0, 0);

        smlt::Viewport minimap_vp(
            smlt::Ratio(0.72f),  // x: right side
            smlt::Ratio(0.72f),  // y: bottom area
            smlt::Ratio(0.25f),  // width: 25% of screen
            smlt::Ratio(0.25f),  // height: 25% of screen
            smlt::Color(0.05f, 0.1f, 0.05f, 1.0f)  // Dark green background
        );

        auto minimap_layer = compositor->create_layer(this, minimap_camera_)
            ->set_viewport(minimap_vp)
            ->set_clear_flags(smlt::BUFFER_CLEAR_ALL)
            ->set_priority(smlt::RENDER_PRIORITY_FOREGROUND)
            ->set_name("minimap");

        link_pipeline(main_layer);
        link_pipeline(minimap_layer);

        // Add some geometry
        auto mesh = assets->create_mesh(smlt::VertexSpecification::DEFAULT);
        mesh->create_submesh_as_box("ground", assets->create_material(), 40, 0.5, 40);
        create_child<smlt::Actor>(mesh);
    }

    void on_update(float dt) override {
        Scene::on_update(dt);
        // Make minimap camera follow the player from above
        if (player_actor_) {
            auto pos = player_actor_->transform->position();
            minimap_camera_->transform->set_position(pos.x, 50, pos.z);
        }
    }

private:
    smlt::CameraPtr main_camera_;
    smlt::CameraPtr minimap_camera_;
    smlt::ActorPtr player_actor_;
};
```

---

## 10. Picture-in-Picture Rear-View Mirror

A rear-view camera rendered to a small rectangular viewport at the top-center of the screen:

```cpp
class RearViewScene : public smlt::Scene {
public:
    RearViewScene(smlt::Window* window) : smlt::Scene(window) {}

    void on_load() override {
        // Main forward-facing camera
        main_camera_ = create_child<smlt::Camera3D>();
        main_camera_->set_perspective_projection(
            smlt::Degrees(60.0f),
            float(window->width()) / float(window->height()),
            0.1f, 1000.0f
        );

        auto main_layer = compositor->create_layer(this, main_camera_)
            ->set_clear_flags(smlt::BUFFER_CLEAR_ALL)
            ->set_name("forward_view");

        // Rear-view camera (positioned behind the player)
        rear_camera_ = create_child<smlt::Camera3D>();
        rear_camera_->transform->set_position(0, 2, 15);
        rear_camera_->transform->look_at(0, 1, 0);

        // Narrow rectangular viewport at top-center
        smlt::Viewport mirror_vp(
            smlt::Ratio(0.35f),   // x: 35% from left
            smlt::Ratio(0.02f),   // y: near the top
            smlt::Ratio(0.3f),    // width: 30% of screen width
            smlt::Ratio(0.12f),   // height: 12% of screen height
            smlt::Color(0.1f, 0.1f, 0.1f, 1.0f)
        );

        auto mirror_layer = compositor->create_layer(this, rear_camera_)
            ->set_viewport(mirror_vp)
            ->set_clear_flags(smlt::BUFFER_CLEAR_ALL)
            ->set_priority(smlt::RENDER_PRIORITY_FOREGROUND)
            ->set_name("rear_mirror");

        link_pipeline(main_layer);
        link_pipeline(mirror_layer);
    }

private:
    smlt::CameraPtr main_camera_;
    smlt::CameraPtr rear_camera_;
};
```

---

## 11. Best Practices

### 1. Always Set Viewport Clear Color

Every viewport needs a clear color. Even if you do not clear the color buffer, the viewport stores the color for potential use. Set it explicitly:

```cpp
smlt::Viewport vp(smlt::VIEWPORT_TYPE_FULL, smlt::Color::black());
layer->set_viewport(vp);
```

### 2. Use the Builder Pattern for Layer Configuration

Chain setters for clarity and brevity:

```cpp
auto layer = compositor->create_layer(this, camera_)
    ->set_viewport(smlt::Viewport(smlt::VIEWPORT_TYPE_FULL, smlt::Color::blue()))
    ->set_clear_flags(smlt::BUFFER_CLEAR_ALL)
    ->set_priority(smlt::RENDER_PRIORITY_MAIN)
    ->set_name("main_3d");

layer->activate();
```

### 3. First Layer Clears, Overlays Do Not

The first layer in your render order should typically clear both color and depth. Overlay layers (UI, picture-in-picture, etc.) that draw on top of existing content should either clear nothing (to composite over the previous output) or clear their own region if they are self-contained:

```cpp
// First layer: clear everything
main_layer->set_clear_flags(smlt::BUFFER_CLEAR_ALL);

// UI overlay: clear nothing
ui_layer->set_clear_flags(0);

// PiP inset: clear its own region
pip_layer->set_clear_flags(smlt::BUFFER_CLEAR_ALL);
```

### 4. Name Your Layers

Layer names make debugging and runtime manipulation much easier:

```cpp
layer->set_name("split_screen_player2");
```

### 5. Use `link_pipeline` for Scene-Managed Layers

```cpp
void on_load() override {
    auto layer = compositor->create_layer(this, camera_);
    link_pipeline(layer);  // Auto-activates/deactivates with the scene
}
```

### 6. Viewport Ratios Auto-Adapt on Resize

Because viewport bounds are stored as ratios, you do not need to recalculate them when the window resizes. The compositor resolves ratios to pixel coordinates each frame based on the current render target size.

### 7. Use Separate Cameras for Separate Viewports

Each viewport/layer needs its own camera. Do not try to share a single camera across multiple viewports -- the camera's view matrix and frustum are single-valued.

### 8. Be Mindful of Render Target Size for Off-Screen Layers

When rendering to a texture, viewport ratios are relative to the **texture size**, not the window size. A viewport with `Ratio(1.0f)` width fills the entire texture, regardless of the window dimensions.
