# Render Pipelines and Compositor

This guide covers the Compositor system, which is Simulant's mechanism for controlling how and where scene content is rendered. Understanding pipelines is essential for building any visible game -- they are the bridge between your scene graph and what appears on screen.

**See also:** [Cameras](../cameras.md), [Scenes](../scene.md), [Stage Nodes](../stage_nodes.md)

---

## 1. What Is the Compositor and Why Does It Exist?

The **Compositor** is the central rendering coordinator in Simulant. It lives on the `Window` and is responsible for:

- Collecting all render pipelines (called **Layers** in the API)
- Sorting them by priority
- Iterating through each active layer each frame and rendering it
- Managing render targets (the screen framebuffer, or off-screen textures)
- Clearing buffers at the right times

### Why a Compositor?

In a simple game you might think "one camera, one screen -- just render it." But real games need more:

- **UI overlays** drawn on top of a 3D world
- **Split-screen** multiplayer with separate viewports
- **Minimaps** or **picture-in-picture** rendered to textures
- **Post-processing** passes that read from and write to intermediate textures
- **Different cameras** viewing different parts of the scene simultaneously

The Compositor solves this by breaking rendering into independent **Layers**, each with its own camera, scene subtree, viewport, and target. You compose these layers together -- like stacking sheets of acetate -- to build the final frame.

### Accessing the Compositor

The Compositor is always available through the Window. Inside a `Scene` subclass you can reach it via the `compositor` convenience pointer:

```cpp
class MyScene : public smlt::Scene {
public:
    MyScene(smlt::Window* window) : smlt::Scene(window) {}

    void on_load() override {
        // Inside a Scene, `compositor` is a shortcut to window->compositor
        auto layer = compositor->create_layer(this, camera_);
    }
};
```

Outside a scene, access it through the window:

```cpp
auto compositor = window->compositor;
```

---

## 2. Creating Render Pipelines

A render pipeline is created by calling `create_layer()` on the Compositor. The method signature is:

```cpp
LayerPtr create_layer(
    StageNode* subtree,          // The stage subtree to render
    CameraPtr camera,             // The camera to view it with
    const Viewport& viewport,     // Viewport region (default = full screen)
    TexturePtr target,            // Render target texture (default = screen)
    int32_t priority = 0          // Render priority (default = 0)
);
```

### Minimal Example

The simplest pipeline attaches a camera to a scene's stage and renders to the full screen:

```cpp
class GameScene : public smlt::Scene {
public:
    GameScene(smlt::Window* window) : smlt::Scene(window) {}

    void on_load() override {
        // Create a camera as a child of the scene
        camera_ = create_child<smlt::Camera3D>();
        camera_->set_perspective_projection(
            Degrees(45.0f),
            float(window->width()) / float(window->height()),
            0.1f, 1000.0f
        );

        // Create a render pipeline: "render this scene through this camera"
        pipeline_ = compositor->create_layer(this, camera_);

        // IMPORTANT: Pipelines start deactivated. Activate it to enable rendering.
        pipeline_->activate();
    }

private:
    smlt::CameraPtr camera_;
    smlt::LayerPtr pipeline_;
};
```

### The Builder Pattern

Every configuration method on `LayerPtr` returns the layer itself, so you can chain calls fluently:

```cpp
auto pipeline = compositor->create_layer(this, camera_)
    ->set_viewport(smlt::Viewport(smlt::VIEWPORT_TYPE_FULL, smlt::Color::blue()))
    ->set_clear_flags(smlt::BUFFER_CLEAR_ALL)
    ->set_priority(smlt::RENDER_PRIORITY_MAIN)
    ->set_name("main_3d");

pipeline->activate();
```

### Important: Pipelines Start Deactivated

Every new layer is created in a **deactivated** state. An inactive layer is skipped during the compositor's render pass. You must call `activate()` for the pipeline to produce any output.

This design prevents a common bug: if you build your scene inside `Scene::on_load()` (which may run on a background thread), an automatically-active pipeline would try to update its stage from the main thread while you are still modifying it, causing data races and crashes.

---

## 3. Layer Configuration

Each layer has several properties you can configure. All of them are settable via the builder-style setters shown above.

### StageNode (the subtree)

The first argument to `create_layer()` is a `StageNode*` -- the root of the subtree to render. This does not have to be the entire scene:

- Pass `this` (the Scene itself) to render the entire scene
- Pass a specific `Stage` or `Actor` to render only that subtree
- This enables effects like rendering a character's portrait in a separate pipeline

```cpp
// Render only a specific actor
auto portrait_pipeline = compositor->create_layer(character_actor, portrait_camera);
```

### Camera

The camera determines the viewpoint, projection, and clipping. You can use any camera type:

- **Camera3D** -- perspective projection for 3D worlds
- **Camera2D** -- orthographic projection for 2D/UI

```cpp
// 3D perspective camera
auto cam3d = create_child<smlt::Camera3D>();
cam3d->set_perspective_projection(Degrees(60.0f), aspect_ratio, 0.1f, 1000.0f);

// 2D orthographic camera (pixel-aligned UI)
auto cam2d = create_child<smlt::Camera2D>();
cam2d->set_orthographic_projection(0, window->width(), 0, window->height());
```

You can swap a layer's camera at runtime:

```cpp
pipeline->set_camera(new_camera);
```

### Viewport

The viewport defines the rectangular region of the render target that the layer draws into. See the [Viewports section below](#11-viewports-and-split-screen) for full details.

```cpp
// Left half of the screen
smlt::Viewport left_viewport(smlt::VIEWPORT_TYPE_VERTICAL_SPLIT_LEFT, smlt::Color::blue());

pipeline->set_viewport(left_viewport);
```

If you do not specify a viewport, the layer renders to the full target area.

### Target Texture

By default a layer renders to the window's framebuffer. You can redirect it to a `Texture` instead -- this is the basis for render-to-texture effects.

```cpp
// Create an off-screen texture to render into
auto offscreen_texture = app->shared_assets->create_texture(512, 512);

auto pip = compositor->create_layer(this, camera_)
    ->set_target(offscreen_texture);
```

See [Section 8: Render to Texture](#8-render-to-texture) for detailed examples.

### Clear Flags

Each layer can specify which buffers to clear before rendering. Common flags:

| Flag | Description |
|------|-------------|
| `BUFFER_CLEAR_COLOR_BUFFER` | Clear the color buffer |
| `BUFFER_CLEAR_DEPTH_BUFFER` | Clear the depth buffer |
| `BUFFER_CLEAR_STENCIL_BUFFER` | Clear the stencil buffer |
| `BUFFER_CLEAR_ALL` | Clear all three buffers |

```cpp
// Clear both color and depth before rendering this layer
pipeline->set_clear_flags(smlt::BUFFER_CLEAR_ALL);
```

The clear color is set on the viewport:

```cpp
smlt::Viewport vp(smlt::VIEWPORT_TYPE_FULL, smlt::Color::black());
pipeline->set_viewport(vp)->set_clear_flags(smlt::BUFFER_CLEAR_ALL);
```

### Detail Level Distances

Layers can control level-of-detail cutoffs for meshes that support multiple detail levels:

```cpp
pipeline->set_detail_level_distances(
    10.0f,   // nearest cutoff
    20.0f,   // near cutoff
    30.0f,   // mid cutoff
    40.0f    // far cutoff
);

// Query what detail level would be used at a given distance
auto level = pipeline->detail_level_at_distance(25.0f);  // DETAIL_LEVEL_MID
```

---

## 4. Render Priorities

When the Compositor renders, it processes layers in **ascending priority order** (lowest number first). This determines draw order: background layers first, then the main scene, then foreground overlays.

### Available Presets

Simulant provides named presets in the `RenderPriority` enum:

```cpp
enum RenderPriorityPreset : int8_t {
    RENDER_PRIORITY_MIN                     = -25,  // Absolute minimum
    RENDER_PRIORITY_ABSOLUTE_BACKGROUND     = -25,
    RENDER_PRIORITY_BACKGROUND            = -10,   // Background elements
    RENDER_PRIORITY_DISTANT               =  -5,   // Distant geometry
    RENDER_PRIORITY_MAIN                  =   0,   // Default / main scene
    RENDER_PRIORITY_NEAR                  =   5,   // Near / foreground objects
    RENDER_PRIORITY_FOREGROUND            =  10,   // UI overlays, HUD
    RENDER_PRIORITY_ABSOLUTE_FOREGROUND   =  25,   // Absolute maximum
    RENDER_PRIORITY_MAX                   =  26    // Upper bound
};
```

### Typical Usage

```cpp
// Background: a skybox or backdrop layer
auto sky_pipeline = compositor->create_layer(sky_stage, sky_camera)
    ->set_priority(smlt::RENDER_PRIORITY_BACKGROUND);

// Main 3D scene (priority 0 is the default)
auto main_pipeline = compositor->create_layer(this, camera_);

// HUD / UI on top of everything
auto hud_pipeline = compositor->create_layer(ui_stage, ui_camera)
    ->set_priority(smlt::RENDER_PRIORITY_FOREGROUND);
```

### Custom Priority Values

The presets are convenient but you are not limited to them. Any `int8_t` value works:

```cpp
// Fine-grained layering
auto layer1 = compositor->create_layer(stage1, cam1)->set_priority(1);
auto layer2 = compositor->create_layer(stage2, cam2)->set_priority(2);
auto layer3 = compositor->create_layer(stage3, cam3)->set_priority(3);
```

### Reading a Layer's Priority

```cpp
int32_t prio = pipeline->priority();
```

---

## 5. Activating and Deactivating Pipelines

### Activation States

| Method | Effect |
|--------|--------|
| `pipeline->activate()` | Enables the layer; it will be rendered each frame |
| `pipeline->deactivate()` | Disables the layer; it will be skipped |
| `pipeline->is_active()` | Returns `true` if the layer is currently active |

### Manual Activation

```cpp
auto pipeline = compositor->create_layer(this, camera_);

// Pipeline is deactivated by default
assert(!pipeline->is_active());

// Enable it
pipeline->activate();
assert(pipeline->is_active());

// Temporarily disable (e.g. pause menu)
pipeline->deactivate();
```

### Automatic Activation Mode

Layers have an **activation mode** that controls whether they auto-activate when their parent scene activates. The default is `LAYER_ACTIVATION_MODE_AUTOMATIC` when using the scene-local `SceneCompositor` (see Section 6).

```cpp
// Manual mode: you control activation yourself
pipeline->set_activation_mode(smlt::LAYER_ACTIVATION_MODE_MANUAL);

// Automatic mode: activates/deactivates with the scene (default for SceneCompositor layers)
pipeline->set_activation_mode(smlt::LAYER_ACTIVATION_MODE_AUTOMATIC);
```

---

## 6. Linking Pipelines to Scenes

When you create a scene, you typically want its render pipelines to activate when the scene activates, and deactivate when the scene deactivates. Doing this manually in every scene's `on_activate()` and `on_deactivate()` methods is repetitive:

```cpp
// The repetitive way (don't do this unless you need custom logic)
void on_activate() override {
    Scene::on_activate();
    pipeline_->activate();
}

void on_deactivate() override {
    Scene::on_deactivate();
    pipeline_->deactivate();
}
```

### Using `link_pipeline`

The `Scene` class provides a helper method that automates this:

```cpp
void on_load() override {
    camera_ = create_child<smlt::Camera3D>();

    pipeline_ = compositor->create_layer(this, camera_);

    // This pipeline will automatically activate when the scene activates
    // and deactivate when the scene deactivates
    link_pipeline(pipeline_);
}
```

With `link_pipeline()`, you do not need to override `on_activate()` or `on_deactivate()` for simple pipeline management.

### SceneCompositor (Scene-Local Compositor)

Simulant also provides a `SceneCompositor` class that wraps the global compositor and manages layers scoped to a single scene. Layers created through `SceneCompositor` are automatically destroyed when the scene is destroyed, and they auto-activate/deactivate with the scene lifecycle:

```cpp
// The scene's `compositor` member is a SceneCompositor
auto layer = compositor->create_layer(stage, camera, smlt::RENDER_PRIORITY_MAIN);
// This layer is tracked by the scene and cleaned up automatically
```

**Recommendation:** Always use the scene's `compositor` pointer (not `window->compositor` directly) when creating layers inside a scene. This ensures proper cleanup and lifecycle management.

---

## 7. Multiple Pipelines: 3D Scene + 2D UI Overlay

A very common pattern is rendering a 3D game world with a 2D heads-up display (HUD) on top. This requires two pipelines:

```cpp
class GameScene : public smlt::Scene {
public:
    GameScene(smlt::Window* window) : smlt::Scene(window) {}

    void on_load() override {
        // ---- 3D world pipeline ----
        camera3d_ = create_child<smlt::Camera3D>();
        camera3d_->set_perspective_projection(
            Degrees(45.0f),
            float(window->width()) / float(window->height()),
            0.1f, 1000.0f
        );
        camera3d_->transform->set_position(0, 5, -20);

        pipeline3d_ = compositor->create_layer(this, camera3d_)
            ->set_clear_flags(smlt::BUFFER_CLEAR_ALL)
            ->set_priority(smlt::RENDER_PRIORITY_MAIN)
            ->set_name("main_3d");

        // ---- 2D UI overlay pipeline ----
        camera2d_ = create_child<smlt::Camera2D>();
        camera2d_->set_orthographic_projection(
            0, window->width(),   // left, right
            0, window->height()   // bottom, top
        );

        pipeline_ui_ = compositor->create_layer(this, camera2d_)
            ->set_clear_flags(0)  // Don't clear -- draw on top of 3D
            ->set_priority(smlt::RENDER_PRIORITY_FOREGROUND)
            ->set_name("ui_overlay");

        // Link both pipelines to this scene
        link_pipeline(pipeline3d_);
        link_pipeline(pipeline_ui_);

        // ---- Add some 3D content ----
        auto mesh = assets->create_mesh(smlt::VertexSpecification::DEFAULT);
        mesh->create_submesh_as_cube("cube", assets->create_material(), 2.0f);
        create_child<smlt::Actor>(mesh);

        // ---- Add some UI content ----
        auto label = create_child<smlt::ui::Label>("Score: 0");
        label->transform->set_position_2d(smlt::Vec2(20, window->height() - 40));
    }

private:
    smlt::CameraPtr camera3d_;
    smlt::CameraPtr camera2d_;
    smlt::LayerPtr pipeline3d_;
    smlt::LayerPtr pipeline_ui_;
};
```

**Key points:**
- The 3D pipeline clears buffers and renders first (priority 0)
- The UI pipeline does **not** clear buffers (`set_clear_flags(0)`) so it draws on top
- The UI pipeline has higher priority (10) so it renders after the 3D scene
- The 2D camera uses orthographic projection for pixel-perfect UI positioning

---

## 8. Render to Texture

By setting a layer's **target** to a `Texture` instead of the default framebuffer, you can render a scene into a texture that can then be displayed elsewhere. Common use cases:

- **Picture-in-picture** -- a secondary camera view drawn as a small inset
- **Minimaps** -- a top-down camera rendered to a texture displayed on the HUD
- **CCTV cameras** -- security cameras rendered to monitors in-game
- **Post-processing** -- render to an intermediate texture, then apply effects

### Basic Render-to-Texture

```cpp
void on_load() override {
    // Create a texture to render into
    auto render_texture = app->shared_assets->create_texture(256, 256);

    // Create a camera for the off-screen view
    auto offscreen_camera = create_child<smlt::Camera3D>();

    // Create a pipeline that renders to the texture
    auto offscreen_pipeline = compositor->create_layer(this, offscreen_camera)
        ->set_target(render_texture)        // Render to texture, not screen
        ->set_clear_flags(smlt::BUFFER_CLEAR_ALL)
        ->set_priority(smlt::RENDER_PRIORITY_BACKGROUND);

    link_pipeline(offscreen_pipeline);

    // Now you can use render_texture as a regular texture
    // (e.g. apply it to a mesh, display it on a UI element, etc.)
    auto screen_mesh = create_quad_with_texture(render_texture);
    auto actor = create_child<smlt::Actor>(screen_mesh);
}
```

### Picture-in-Picture Example

```cpp
void on_load() override {
    // Main 3D camera
    main_camera_ = create_child<smlt::Camera3D>();
    main_camera_->set_perspective_projection(Degrees(60.0f), aspect, 0.1f, 1000.0f);

    auto main_pipeline = compositor->create_layer(this, main_camera_)
        ->set_clear_flags(smlt::BUFFER_CLEAR_ALL)
        ->set_name("main");

    // Secondary "rear-view mirror" camera
    mirror_camera_ = create_child<smlt::Camera3D>();
    mirror_camera_->transform->set_position(0, 2, 10);  // Behind the player

    // Render mirror view to a small texture
    auto mirror_tex = app->shared_assets->create_texture(256, 128);

    auto mirror_pipeline = compositor->create_layer(this, mirror_camera_)
        ->set_target(mirror_tex)
        ->set_clear_flags(smlt::BUFFER_CLEAR_ALL)
        ->set_priority(smlt::RENDER_PRIORITY_BACKGROUND)
        ->set_name("mirror");

    // UI overlay to display the mirror texture
    ui_camera_ = create_child<smlt::Camera2D>();
    ui_camera_->set_orthographic_projection(0, window->width(), 0, window->height());

    auto ui_pipeline = compositor->create_layer(this, ui_camera_)
        ->set_clear_flags(0)
        ->set_priority(smlt::RENDER_PRIORITY_FOREGROUND)
        ->set_name("ui");

    // Create a UI image showing the mirror texture
    auto mirror_image = create_child<smlt::ui::Image>(mirror_tex);
    mirror_image->set_anchor_point(1.0f, 0.0f);  // Top-right corner
    mirror_image->transform->set_position_2d(
        smlt::Vec2(window->width() - 270, window->height() - 140)
    );

    link_pipeline(main_pipeline);
    link_pipeline(mirror_pipeline);
    link_pipeline(ui_pipeline);
}
```

### CCTV Camera Example

```cpp
void on_load() override {
    // Main scene pipeline
    main_camera_ = create_child<smlt::Camera3D>();
    auto main_pipeline = compositor->create_layer(this, main_camera_)
        ->set_clear_flags(smlt::BUFFER_CLEAR_ALL)
        ->set_name("main");

    // CCTV camera in the world
    auto cctv_camera = create_child<smlt::Camera3D>();
    cctv_camera->transform->set_position(0, 15, 0);
    cctv_camera->transform->look_at(smlt::Vec3(0, 0, 0));

    auto cctv_texture = app->shared_assets->create_texture(512, 512);

    auto cctv_pipeline = compositor->create_layer(this, cctv_camera)
        ->set_target(cctv_texture)
        ->set_clear_flags(smlt::BUFFER_CLEAR_ALL)
        ->set_name("cctv");

    link_pipeline(main_pipeline);
    link_pipeline(cctv_pipeline);

    // In the world, place a "monitor" actor that displays this texture
    auto monitor_material = assets->create_material();
    monitor_material->set_base_color_map(cctv_texture);
    monitor_material->set_lighting_enabled(false);  // Self-illuminated screen

    auto monitor_mesh = assets->create_mesh(smlt::VertexSpecification::DEFAULT);
    monitor_mesh->create_submesh_as_plane("screen", monitor_material, 4.0f, 3.0f);

    auto monitor = create_child<smlt::Actor>(monitor_mesh);
    monitor->transform->set_position(5, 3, 0);
    monitor->transform->set_rotation(smlt::Vec3(0, -90, 0));
}
```

---

## 9. Pipeline Naming and Lookup

You can assign names to layers and look them up later. This is useful when you need to access a pipeline from code that did not create it.

```cpp
// Set a name when creating
compositor->create_layer(this, camera_)
    ->set_name("main_3d");

// Look it up later by name
auto found = compositor->find_layer("main_3d");
if (found) {
    found->deactivate();
}

// Check existence
if (compositor->has_layer("main_3d")) {
    // ...
}
```

If no layer with the given name exists, `find_layer()` returns a null `LayerPtr`.

**Note:** Names do not have to be unique. If multiple layers share the same name, `find_layer()` returns the first match it finds. For reliable lookups, use distinct names.

---

## 10. Pipeline Lifecycle and Scene Transitions

### Creation and Destruction

Layers are created via `create_layer()` and destroyed via `destroy()`:

```cpp
auto pipeline = compositor->create_layer(this, camera_);

// When done with the pipeline
pipeline->destroy();
```

After calling `destroy()`, the layer is queued for cleanup. It is automatically deactivated and removed from the compositor's render list during the next frame's cleanup phase.

### Scene Transitions

When using Simulant's `SceneManager` to transition between scenes, pipelines created through the scene's `compositor` (a `SceneCompositor`) are managed automatically:

1. **Scene activates** -> Layers with `LAYER_ACTIVATION_MODE_AUTOMATIC` are activated
2. **Scene deactivates** -> Those same layers are deactivated
3. **Scene unloads** -> All scene layers are destroyed

If you create layers directly on `window->compositor`, you are responsible for their lifecycle across scene transitions.

### Best Practice: Use Scene-Local Compositor

```cpp
// GOOD: inside a Scene, use the scene's compositor (SceneCompositor)
void on_load() override {
    auto layer = compositor->create_layer(this, camera_);
    link_pipeline(layer);  // Auto-activates with the scene
}

// RISKY: creating layers on the global compositor from within a scene
void on_load() override {
    auto layer = window->compositor->create_layer(this, camera_);
    // You must manually manage this layer's lifecycle
}
```

---

## 11. Viewports and Split-Screen

A viewport defines the rectangular region within a render target that a layer draws into. Simulant provides several built-in viewport types for common layouts.

### Viewport Types

| Viewport Type | Description |
|---------------|-------------|
| `VIEWPORT_TYPE_FULL` | Fills the entire target (default) |
| `VIEWPORT_TYPE_VERTICAL_SPLIT_LEFT` | Left half of the screen |
| `VIEWPORT_TYPE_VERTICAL_SPLIT_RIGHT` | Right half of the screen |
| `VIEWPORT_TYPE_HORIZONTAL_SPLIT_TOP` | Top half of the screen |
| `VIEWPORT_TYPE_HORIZONTAL_SPLIT_BOTTOM` | Bottom half of the screen |
| `VIEWPORT_TYPE_BLACKBAR_4_BY_3` | 4:3 letterboxed on wider screens |
| `VIEWPORT_TYPE_BLACKBAR_16_BY_9` | 16:9 letterboxed |
| `VIEWPORT_TYPE_BLACKBAR_16_BY_10` | 16:10 letterboxed |
| `VIEWPORT_TYPE_CUSTOM` | Define your own region with ratios |

### Split-Screen Two Players

```cpp
class SplitScreenScene : public smlt::Scene {
public:
    SplitScreenScene(smlt::Window* window) : smlt::Scene(window) {}

    void on_load() override {
        // Two cameras, one for each player
        auto camera1 = create_child<smlt::Camera3D>();
        auto camera2 = create_child<smlt::Camera3D>();

        camera1->transform->set_position(-5, 5, -10);
        camera2->transform->set_position(5, 5, -10);

        // Left viewport (Player 1) - red clear color
        smlt::Viewport left_vp(smlt::VIEWPORT_TYPE_VERTICAL_SPLIT_LEFT, smlt::Color::from_rgb(0.2f, 0.0f, 0.0f));

        // Right viewport (Player 2) - green clear color
        smlt::Viewport right_vp(smlt::VIEWPORT_TYPE_VERTICAL_SPLIT_RIGHT, smlt::Color::from_rgb(0.0f, 0.2f, 0.0f));

        // Each player gets their own layer with its own viewport
        compositor->create_layer(this, camera1)
            ->set_viewport(left_vp)
            ->set_clear_flags(smlt::BUFFER_CLEAR_ALL)
            ->set_name("player1");

        compositor->create_layer(this, camera2)
            ->set_viewport(right_vp)
            ->set_clear_flags(smlt::BUFFER_CLEAR_ALL)
            ->set_name("player2");

        // Add some geometry to see
        auto mesh = assets->create_mesh(smlt::VertexSpecification::DEFAULT);
        mesh->create_submesh_as_cube("cube", assets->create_material(), 2.0f);
        create_child<smlt::Actor>(mesh);
    }
};
```

### Custom Viewports with Ratios

For arbitrary positioning and sizing, use `VIEWPORT_TYPE_CUSTOM` with ratios (values from 0.0 to 1.0):

```cpp
// A small viewport in the bottom-left corner
smlt::Viewport minimap_vp(
    smlt::Ratio(0.0f),   // x: 0% from left
    smlt::Ratio(0.7f),   // y: 70% from top (bottom area)
    smlt::Ratio(0.25f),  // width: 25% of screen width
    smlt::Ratio(0.25f),  // height: 25% of screen height
    smlt::Color::black()
);

compositor->create_layer(this, minimap_camera)
    ->set_viewport(minimap_vp)
    ->set_clear_flags(smlt::BUFFER_CLEAR_ALL);
```

The `Ratio` type stores fractional positions that automatically scale to the current render target size.

---

## 12. Best Practices for Organizing Render Pipelines

### 1. Always use the Scene's Compositor

Inside a `Scene`, use the `compositor` member (which is a `SceneCompositor`) rather than `window->compositor` directly. This ensures automatic cleanup and proper lifecycle management.

### 2. Name Your Pipelines

Naming makes debugging, profiling, and runtime manipulation much easier:

```cpp
auto layer = compositor->create_layer(this, camera_)
    ->set_name("main_3d");
```

### 3. Use `link_pipeline` in `on_load()`

Avoid overriding `on_activate()`/`on_deactivate()` just to toggle pipelines. Use `link_pipeline()` instead.

### 4. Separate Concerns with Multiple Pipelines

If you need different rendering behaviors (3D world, UI, post-processing, minimap), use separate layers. Do not try to multiplex everything through a single pipeline.

### 5. Clear Buffers Selectively

- The **first** pipeline in your render order should clear both color and depth (`BUFFER_CLEAR_ALL`)
- **Overlay** pipelines (UI, HUD) should typically clear nothing (`set_clear_flags(0)`) so they draw on top of existing content
- **Render-to-texture** pipelines should always clear their target

### 6. Use Priority Presets for Readability

Prefer named presets over magic numbers:

```cpp
// Clear and readable
->set_priority(smlt::RENDER_PRIORITY_FOREGROUND)

// Opaque and harder to understand
->set_priority(10)
```

### 7. Manage Pipeline References Carefully

Store `LayerPtr` as member variables if you need to access them later. If you only need temporary configuration at creation time, you can discard the pointer after calling `link_pipeline()`:

```cpp
// You don't need to store this if you link it
compositor->create_layer(this, camera_)
    ->set_clear_flags(smlt::BUFFER_CLEAR_ALL)
    ->set_name("main");
link_pipeline_by_name("main");
```

---

## 13. Complete Examples

### Example 1: Basic 3D Scene

```cpp
#include <simulant/simulant.h>

class MainScene : public smlt::Scene {
public:
    MainScene(smlt::Window* window) : smlt::Scene(window) {}

    void on_load() override {
        // 1. Create a 3D camera
        camera_ = create_child<smlt::Camera3D>();
        camera_->set_perspective_projection(
            Degrees(45.0f),
            float(window->width()) / float(window->height()),
            0.1f, 1000.0f
        );
        camera_->transform->set_position(0, 3, -10);
        camera_->transform->look_at(smlt::Vec3(0, 0, 0));

        // 2. Create the render pipeline
        pipeline_ = compositor->create_layer(this, camera_)
            ->set_clear_flags(smlt::BUFFER_CLEAR_ALL)
            ->set_priority(smlt::RENDER_PRIORITY_MAIN)
            ->set_name("main_3d");

        // 3. Link to the scene for automatic activation
        link_pipeline(pipeline_);

        // 4. Add some content
        auto mesh = assets->create_mesh(smlt::VertexSpecification::DEFAULT);
        mesh->create_submesh_as_cube("cube", assets->create_material(), 2.0f);
        auto cube = create_child<smlt::Actor>(mesh);
        cube->transform->set_position(0, 0, 0);

        // 5. Add lighting
        lighting->set_ambient_light(smlt::Color(0.3f, 0.3f, 0.3f, 1.0f));
        auto light = create_child<smlt::DirectionalLight>();
        light->set_direction(smlt::Vec3(-1, -1, 0).normalized());
        light->set_color(smlt::Color::white());
    }

    void on_update(float dt) override {
        Scene::on_update(dt);
        // Simple rotation animation
        if (auto cube = find_child_by_name<smlt::Actor>("cube")) {
            cube->transform->rotate(smlt::Vec3::up(), smlt::Degrees(45.0f * dt));
        }
    }

private:
    smlt::CameraPtr camera_;
    smlt::LayerPtr pipeline_;
};

class BasicApp : public smlt::Application {
public:
    BasicApp(const smlt::AppConfig& config) : smlt::Application(config) {}

    bool init() override {
        scenes->register_scene<MainScene>("main");
        scenes->activate("main");
        return true;
    }
};

int main(int argc, char* argv[]) {
    smlt::AppConfig config;
    config.title = "Basic 3D Scene";
    config.width = 1280;
    config.height = 720;
    config.fullscreen = false;

    BasicApp app(config);
    return app.run();
}
```

### Example 2: 3D Game with HUD Overlay

```cpp
#include <simulant/simulant.h>

class GameScene : public smlt::Scene {
public:
    GameScene(smlt::Window* window) : smlt::Scene(window) {}

    void on_load() override {
        setup_3d_pipeline();
        setup_hud_pipeline();
        setup_game_world();
    }

    void on_update(float dt) override {
        Scene::on_update(dt);
        score_ += dt * 10.0f;

        if (auto label = find_child_by_name<smlt::ui::Label>("score_label")) {
            label->set_text("Score: " + std::to_string((int)score_));
        }
    }

private:
    void setup_3d_pipeline() {
        camera3d_ = create_child<smlt::Camera3D>();
        camera3d_->set_perspective_projection(
            Degrees(60.0f),
            float(window->width()) / float(window->height()),
            0.1f, 1000.0f
        );
        camera3d_->transform->set_position(0, 5, -15);

        compositor->create_layer(this, camera3d_)
            ->set_clear_flags(smlt::BUFFER_CLEAR_ALL)
            ->set_priority(smlt::RENDER_PRIORITY_MAIN)
            ->set_name("game_world");
    }

    void setup_hud_pipeline() {
        // Orthographic camera for 2D overlay
        hud_camera_ = create_child<smlt::Camera2D>();
        hud_camera_->set_orthographic_projection(
            0, window->width(), 0, window->height()
        );

        // HUD pipeline: NO clear (draws on top), FOREGROUND priority
        compositor->create_layer(this, hud_camera_)
            ->set_clear_flags(0)
            ->set_priority(smlt::RENDER_PRIORITY_FOREGROUND)
            ->set_name("hud");

        // Add HUD elements
        auto score_label = create_child<smlt::ui::Label>("Score: 0");
        score_label->set_name("score_label");
        score_label->set_anchor_point(0.0f, 1.0f);
        score_label->transform->set_position_2d(smlt::Vec2(20, window->height() - 20));

        auto health_bar = create_child<smlt::ui::ProgressBar>();
        health_bar->set_name("health_bar");
        health_bar->set_text("Health");
        health_bar->set_value(100.0f);
        health_bar->set_anchor_point(1.0f, 1.0f);
        health_bar->transform->set_position_2d(
            smlt::Vec2(window->width() - 220, window->height() - 20)
        );
    }

    void setup_game_world() {
        lighting->set_ambient_light(smlt::Color(0.4f, 0.4f, 0.4f, 1.0f));

        // Ground plane
        auto ground_mat = assets->create_material();
        ground_mat->set_base_color(smlt::Color(0.2f, 0.5f, 0.2f, 1.0f));

        auto ground_mesh = assets->create_mesh(smlt::VertexSpecification::DEFAULT);
        ground_mesh->create_submesh_as_box("ground", ground_mat, 100, 0.5f, 100);
        auto ground = create_child<smlt::Actor>(ground_mesh);
        ground->transform->set_position(0, -1, 0);

        // A few cubes scattered around
        for (int i = 0; i < 5; i++) {
            auto mesh = assets->create_mesh(smlt::VertexSpecification::DEFAULT);
            mesh->create_submesh_as_cube("cube", assets->create_material(), 1.0f);
            auto cube = create_child<smlt::Actor>(mesh);
            cube->transform->set_position((i - 2) * 4, 0.5f, -5);
        }
    }

    smlt::CameraPtr camera3d_;
    smlt::CameraPtr hud_camera_;
    float score_ = 0;
};

class GameApp : public smlt::Application {
public:
    GameApp(const smlt::AppConfig& config) : smlt::Application(config) {}

    bool init() override {
        scenes->register_scene<GameScene>("game");
        scenes->activate("game");
        return true;
    }
};

int main(int argc, char* argv[]) {
    smlt::AppConfig config;
    config.title = "3D Game with HUD";
    config.width = 1280;
    config.height = 720;
    config.fullscreen = false;

    GameApp app(config);
    return app.run();
}
```

### Example 3: Split-Screen Multiplayer

```cpp
#include <simulant/simulant.h>

class SplitScreenScene : public smlt::Scene {
public:
    SplitScreenScene(smlt::Window* window) : smlt::Scene(window) {}

    void on_load() override {
        // Player 1's camera
        camera_p1_ = create_child<smlt::Camera3D>();
        camera_p1_->set_perspective_projection(Degrees(60.0f), aspect(), 0.1f, 1000.0f);
        camera_p1_->transform->set_position(-3, 3, -8);

        // Player 2's camera
        camera_p2_ = create_child<smlt::Camera3D>();
        camera_p2_->set_perspective_projection(Degrees(60.0f), aspect(), 0.1f, 1000.0f);
        camera_p2_->transform->set_position(3, 3, -8);

        // Left viewport for Player 1
        smlt::Viewport left_vp(smlt::VIEWPORT_TYPE_VERTICAL_SPLIT_LEFT, smlt::Color(0.1f, 0.1f, 0.2f));

        // Right viewport for Player 2
        smlt::Viewport right_vp(smlt::VIEWPORT_TYPE_VERTICAL_SPLIT_RIGHT, smlt::Color(0.2f, 0.1f, 0.1f));

        // Create pipelines
        compositor->create_layer(this, camera_p1_)
            ->set_viewport(left_vp)
            ->set_clear_flags(smlt::BUFFER_CLEAR_ALL)
            ->set_name("player1");

        compositor->create_layer(this, camera_p2_)
            ->set_viewport(right_vp)
            ->set_clear_flags(smlt::BUFFER_CLEAR_ALL)
            ->set_name("player2");

        // Shared game world content (both cameras see the same scene)
        auto mesh = assets->create_mesh(smlt::VertexSpecification::DEFAULT);
        mesh->create_submesh_as_cube("cube", assets->create_material(), 2.0f);
        create_child<smlt::Actor>(mesh);

        lighting->set_ambient_light(smlt::Color(0.3f, 0.3f, 0.3f, 1.0f));
        auto light = create_child<smlt::DirectionalLight>();
        light->set_direction(smlt::Vec3(-1, -1, -1).normalized());
    }

private:
    float aspect() const {
        return (float(window->width()) / 2.0f) / float(window->height());
    }

    smlt::CameraPtr camera_p1_;
    smlt::CameraPtr camera_p2_;
};

class App : public smlt::Application {
public:
    App(const smlt::AppConfig& config) : smlt::Application(config) {}

    bool init() override {
        scenes->register_scene<SplitScreenScene>("splitscreen");
        scenes->activate("splitscreen");
        return true;
    }
};

int main(int argc, char* argv[]) {
    smlt::AppConfig config;
    config.title = "Split-Screen";
    config.width = 1280;
    config.height = 720;
    config.fullscreen = false;

    App app(config);
    return app.run();
}
```

---

## API Quick Reference

### Compositor Methods

| Method | Description |
|--------|-------------|
| `create_layer(stage, camera, viewport, target, priority)` | Create a new render pipeline (Layer) |
| `find_layer(name)` | Look up a layer by name |
| `has_layer(name)` | Check if a named layer exists |
| `destroy_all_layers()` | Destroy all layers |
| `destroy_object(layer)` | Queue a layer for destruction |

### Layer Methods

| Method | Description |
|--------|-------------|
| `activate()` | Enable the layer for rendering |
| `deactivate()` | Disable the layer |
| `is_active()` | Check if the layer is active |
| `set_priority(int32_t)` | Set render priority (returns `LayerPtr` for chaining) |
| `priority()` | Get the current render priority |
| `set_viewport(Viewport)` | Set the viewport region (chainable) |
| `set_target(TexturePtr)` | Set the render target texture (chainable) |
| `set_clear_flags(uint32_t)` | Set buffer clear flags (chainable) |
| `set_name(string)` | Set the layer name (chainable) |
| `name()` | Get the layer name |
| `camera()` | Get the attached camera |
| `stage_node()` | Get the attached stage subtree |
| `target()` | Get the target texture (nullptr = framebuffer) |
| `clear_flags()` | Get the clear flags |
| `set_camera(CameraPtr)` | Replace the camera |
| `set_detail_level_distances(...)` | Set LOD cutoff distances |
| `set_activation_mode(mode)` | Set automatic or manual activation |

### Scene Methods

| Method | Description |
|--------|-------------|
| `link_pipeline(layer)` | Link a layer to this scene for automatic activation |
| `compositor` | The SceneCompositor for this scene |

---

## Related Documentation

- [Cameras](../cameras.md) -- Camera types, projection modes, and configuration
- [Scenes](../scene.md) -- Scene lifecycle, SceneManager, and transitions
- [Stage Nodes](../stage_nodes.md) -- The scene graph hierarchy
- [Textures](textures.md) -- Creating and using textures for render targets
