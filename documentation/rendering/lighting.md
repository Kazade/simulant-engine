# Lighting

This guide covers the lighting system in Simulant. Lights are `StageNode` subclasses that you place in your scene to illuminate [Actors](../core-concepts/actors.md) and other geometry. Lighting interacts with [Materials](materials.md) and is processed by the [Render Pipeline](pipelines.md) during the compositor's render pass.

**See also:** [Materials](materials.md), [Actors](../core-concepts/actors.md), [Render Pipelines](pipelines.md), [Stage Nodes](stage_nodes.md)

---

## 1. Overview of Lighting in Simulant

Simulant uses a fixed-per-object lighting model. Each renderable (typically an `Actor`) is lit by up to **8 lights** simultaneously on desktop platforms (4 on PSP, 2 on Dreamcast). Lights are collected per-frame, culled by distance, and sorted by proximity before being passed to the renderer.

Key points:

- Lights are `StageNode` objects -- they participate in the scene graph, can have parents, and can be moved like any other node
- There is no global "light manager"; you create lights as children of your `Scene` or any `StageNode`
- A scene has a `LightingSettings` object accessible via the `lighting` convenience pointer, used to set ambient light
- Lighting is applied per-material. Materials have an `s_lighting_enabled` property that controls whether they respond to lights (default: enabled)
- The engine supports two OpenGL render paths: **GL1x** (fixed-function OpenGL 1.x-style) and **GL2x** (programmable shaders). Both support the same light API, but the shading model differs

---

## 2. Light Types

Simulant defines three light types in the `LightType` enum:

| Type | Class | Description |
|------|-------|-------------|
| `LIGHT_TYPE_POINT` | `PointLight` | Omnidirectional light at a position. Intensity falls off with distance. |
| `LIGHT_TYPE_DIRECTIONAL` | `DirectionalLight` | Uniform light from a direction. No attenuation. Simulates distant sunlight. |
| `LIGHT_TYPE_SPOT_LIGHT` | (not yet exposed as a standalone class) | Defined in the enum but not yet implemented as a user-facing class. |

### PointLight

A `PointLight` emits light equally in all directions from a single point in space. It has a **range** property that defines the maximum distance its influence extends. Beyond this range, the light contributes zero illumination.

Use point lights for lamps, torches, explosions, magic effects, and any localized light source.

### DirectionalLight

A `DirectionalLight` has no position -- only a direction. Every surface in the scene receives light from the same direction, with no attenuation. This models light sources so far away that their rays are effectively parallel (e.g. the sun).

Use directional lights for sunlight, moonlight, and large-scale ambient direction.

### Ambient Light (Scene-Level)

In addition to light nodes, the `Scene` has an ambient light setting accessible through `lighting->set_ambient_light()`. This provides a uniform baseline illumination to all surfaces, simulating indirect/bounced light. Set this to a dim color to prevent completely dark shadows.

---

## 3. Creating and Positioning Lights

### Creating a PointLight

```cpp
class GameScene : public smlt::Scene {
public:
    GameScene(smlt::Window* window) : smlt::Scene(window) {}

    void on_load() override {
        // Set a dim ambient light so nothing is completely dark
        lighting->set_ambient_light(smlt::Color(0.2f, 0.2f, 0.2f, 1.0f));

        // Create a point light at the origin
        auto lamp = create_child<smlt::PointLight>();
        lamp->transform->set_position(0, 5, 0);
        lamp->set_color(smlt::Color::white());
        lamp->set_intensity(1000.0f);
        lamp->set_range(50.0f);
    }
};
```

### Creating a PointLight with Params

`PointLight` accepts a `position` parameter via the `Params` constructor:

```cpp
auto light = create_child<smlt::PointLight>(
    smlt::Params()
        .set("position", smlt::Vec3(10, 3, -5))
        .set("color", smlt::Color(1, 0.5f, 0.2f, 1))
);
```

### Creating a DirectionalLight

```cpp
auto sun = create_child<smlt::DirectionalLight>(
    smlt::Params()
        .set("direction", smlt::Vec3(1, -1, 0))  // Direction the light is pointing
        .set("color", smlt::Color(1, 0.9f, 0.7f, 1))
);
```

The direction vector does not need to be normalized. The light type is automatically set to `LIGHT_TYPE_DIRECTIONAL`.

### Attaching a Light to an Actor

Because lights are `StageNode` objects, you can parent them to actors. This is useful for lights that should follow a character or object:

```cpp
auto player = create_child<smlt::Actor>(player_mesh);

// A torch light that follows the player
auto torch_light = create_child<smlt::PointLight>();
torch_light->set_parent(player);
torch_light->transform->set_position(0, 2, 0);  // Offset from player's origin
torch_light->set_color(smlt::Color(1, 0.6f, 0.2f, 1));
torch_light->set_intensity(500.0f);
torch_light->set_range(20.0f);
```

Now wherever the player moves, the torch light follows.

---

## 4. Light Properties

Every light has these core properties, accessible via getters and setters on the `Light` base class:

### Color

```cpp
light->set_color(smlt::Color(1.0f, 0.5f, 0.2f, 1.0f));  // Warm orange
auto c = light->color();
```

Colors are standard RGBA. The alpha channel is not used in lighting calculations. Values can exceed 1.0 for HDR-style brightness.

### Intensity

```cpp
light->set_intensity(1000.0f);
float i = light->intensity();
```

Intensity is a scalar multiplier applied to the light's color. Higher values produce brighter illumination. Typical values range from hundreds to thousands depending on your scene scale.

> **Note:** In the GL1x renderer, intensity is combined with color to produce the final diffuse, ambient, and specular terms sent to OpenGL. In GL2x, intensity is passed as a separate uniform (`s_light_intensity`) to the shader.

### Range

```cpp
light->set_range(50.0f);  // Light influences objects within 50 units
float r = light->range();
```

Range only applies to `PointLight`. It defines the maximum distance at which the light affects geometry. Directional lights have no range (they affect everything).

> **Important:** Range is also used for light culling. If a renderable's center is outside the light's range, it will not receive light from that source.

### Direction (DirectionalLight only)

```cpp
directional_light->set_direction(1, -0.5f, 0);  // Pointing down and to the right
smlt::Vec3 dir = directional_light->direction();
```

Direction is stored internally as the negated position in the light's transform. Calling `set_direction()` automatically sets the light type to `LIGHT_TYPE_DIRECTIONAL`.

---

## 5. Per-Pixel vs Per-Vertex Lighting

The shading granularity depends on which renderer is active:

### GL1x Renderer (Fixed-Function)

The GL1x renderer uses OpenGL's fixed-function lighting pipeline. Lighting is computed **per-vertex** and then interpolated across the face. This is fast but can produce visible artifacts on low-polygon geometry, particularly specular highlights that "swim" across surfaces.

Attenuation in GL1x uses a quadratic approximation:

```
attenuation = max(min(1.0 - (distance / range)^4, 1.0), 0.0) / distance^2
```

This is implemented by setting OpenGL light attenuation constants to:
- `GL_CONSTANT_ATTENUATION = 0.1`
- `GL_LINEAR_ATTENUATION = -1.0 / range`
- `GL_QUADRATIC_ATTENUATION = 1.0`

### GL2x Renderer (Programmable Shaders)

The GL2x renderer passes light data (position, color, intensity, range) as uniforms to custom shaders. The actual lighting calculation happens in your fragment shader, enabling **per-pixel** lighting if your shader implements it.

The following uniforms are available in GL2x shaders (indexed by light ID, 0 to N-1):

| Uniform | Type | Description |
|---------|------|-------------|
| `s_light_position[0]` | `vec4` | World position. `w = 0` for directional, `w = 1` for point lights |
| `s_light_color[0]` | `vec4` | Light color (RGBA) |
| `s_light_intensity[0]` | `float` | Light intensity multiplier |
| `s_light_range[0]` | `float` | Light range (point lights only) |

For the first light (ID 0), both indexed and non-indexed uniform names are available (e.g., `s_light_position` and `s_light_position[0]` both work).

A minimal fragment shader lighting calculation might look like:

```glsl
uniform vec4 s_light_position;
uniform vec4 s_light_color;
uniform float s_light_intensity;
uniform float s_light_range;

// In your main()
vec3 lightDir;
float attenuation = 1.0;

if (s_light_position.w == 0.0) {
    // Directional light
    lightDir = normalize(s_light_position.xyz);
} else {
    // Point light
    vec3 lightPos = s_light_position.xyz;
    lightDir = lightPos - worldPos;
    float dist = length(lightDir);
    lightDir = normalize(lightDir);
    attenuation = max(1.0 - pow(dist / s_light_range, 4.0), 0.0) / (dist * dist + 0.001);
}

float diff = max(dot(normalize(normal), lightDir), 0.0);
vec3 diffuse = diff * s_light_color.rgb * s_light_intensity * attenuation;
```

### Which Should You Use?

- **GL1x** is simpler and works on older hardware. Suitable for prototypes and retro-style games.
- **GL2x** gives you full control over the lighting model. Recommended for most projects.

You can force a specific renderer via the `SIMULANT_RENDERER` environment variable or `config.development.force_renderer` in your `AppConfig`.

---

## 6. Lighting in GL1x vs GL2x Renderers

Both renderers support the same light creation API, but differ in how they process lights:

| Feature | GL1x | GL2x |
|---------|------|------|
| Shading model | Per-vertex (fixed-function) | Defined by your shader (typically per-pixel) |
| Light uniforms | OpenGL `GL_LIGHT0`..`GL_LIGHTn` | Custom `s_light_*` uniforms |
| Attenuation | Fixed quadratic approximation | Your shader controls |
| Ambient light | Set via `lighting->set_ambient_light()` | Passed to scene; shader must use it |
| Material lighting toggle | `material->set_lighting_enabled(false)` | Same |
| Max lights per renderable | 8 (desktop), 4 (PSP), 2 (Dreamcast) | Same |

### Forcing a Renderer

```cpp
smlt::AppConfig config;
config.development.force_renderer = "gl1x";  // or "gl2x"
```

Or set the environment variable before running:

```bash
SIMULANT_RENDERER=gl1x ./my_game
```

---

## 7. Shadow Mapping

> **TODO:** Shadow mapping is not yet implemented in Simulant.

The engine does include a **shadow volume** system (`ShadowVolumeManager` and `MeshSilhouette`) that calculates stencil-based shadow volumes from mesh silhouettes. This system:

- Works with both `PointLight` and `DirectionalLight`
- Calculates silhouette edges by comparing face normals against the light direction/position
- Caches silhouette data and only recalculates when the mesh or light moves
- Supports two methods: `SHADOW_METHOD_STENCIL_DEPTH_FAIL` (standard) and `SHADOW_METHOD_STENCIL_EXCLUSIVE_OR`

However, the shadow volume system is **not yet wired into the main render pipeline**. It exists as infrastructure but is not actively used during rendering.

For now, if you need shadows, consider:
- Baking shadow maps into your textures
- Using dark transparent planes under characters as "fake" shadows
- Implementing shadow volumes in custom GL2x shaders

---

## 8. Multiple Lights and Light Accumulation

Simulant supports multiple lights affecting the same renderable. The engine handles light collection, sorting, and accumulation automatically.

### How Light Selection Works

Each frame, the compositor:

1. **Collects visible Lights**: All lights in the scene (that are visible and not culled) are gathered
2. **Sorts by Distance**: Lights are sorted by their distance to the renderable's center. Directional lights always sort first (highest priority)
3. **Takes the N Closest**: The top `MAX_LIGHTS_PER_RENDERABLE` lights are selected (8 on desktop)
4. **Passes to Renderer**: These lights are sent to the renderer as uniforms (GL2x) or OpenGL light slots (GL1x)

### Light Sorting Priority

Directional lights always take priority over point lights. Among lights of the same type, the closest lights to the renderable's center are chosen:

```cpp
// From the compositor's sorting logic:
if (lhs->light_type() == LIGHT_TYPE_DIRECTIONAL &&
    rhs->light_type() != LIGHT_TYPE_DIRECTIONAL) {
    return true;  // Directional always wins
}
float lhs_dist = (node->center() - lhs->transform->position()).length_squared();
float rhs_dist = (node->center() - rhs->transform->position()).length_squared();
return lhs_dist < rhs_dist;
```

> **Caveat:** Sorting by the renderable's center point is a simplification. Large objects may have surfaces that should be lit by a light just outside the center-based cutoff. This is a known limitation noted in the source code.

### Example: Three-Colored Lights on a Single Object

```cpp
class MultiLightScene : public smlt::Scene {
public:
    MultiLightScene(smlt::Window* window) : smlt::Scene(window) {}

    void on_load() override {
        lighting->set_ambient_light(smlt::Color(0.1f, 0.1f, 0.1f, 1.0f));

        camera_ = create_child<smlt::Camera3D>();
        camera_->set_perspective_projection(
            smlt::Degrees(45.0f),
            float(window->width()) / float(window->height()),
            0.1f, 100.0f
        );
        camera_->transform->set_position(0, 5, -15);
        camera_->transform->look_at(0, 0, 0);

        auto pipeline = compositor->create_layer(this, camera_)
            ->set_clear_flags(smlt::BUFFER_CLEAR_ALL);
        link_pipeline(pipeline);

        // Central sphere to illuminate
        auto mat = assets->create_material();
        auto mesh = assets->create_mesh(smlt::VertexSpecification::DEFAULT);
        mesh->create_submesh_as_sphere("sphere", mat, 1.5f);
        auto actor = create_child<smlt::Actor>(mesh);

        // Red light from the left
        auto red_light = create_child<smlt::PointLight>();
        red_light->transform->set_position(-5, 3, 0);
        red_light->set_color(smlt::Color(1, 0, 0, 1));
        red_light->set_intensity(500.0f);
        red_light->set_range(15.0f);

        // Green light from the right
        auto green_light = create_child<smlt::PointLight>();
        green_light->transform->set_position(5, 3, 0);
        green_light->set_color(smlt::Color(0, 1, 0, 1));
        green_light->set_intensity(500.0f);
        green_light->set_range(15.0f);

        // Blue light from above
        auto blue_light = create_child<smlt::PointLight>();
        blue_light->transform->set_position(0, 8, 0);
        blue_light->set_color(smlt::Color(0, 0, 1, 1));
        blue_light->set_intensity(500.0f);
        blue_light->set_range(15.0f);
    }

private:
    smlt::CameraPtr camera_;
};
```

---

## 9. Light Culling and Performance

### Automatic Culling

Point lights are **cullable by default**. If a point light is outside the camera's view frustum (as determined by the scene partitioner), it will be skipped during light collection. Directional lights are **never culled** (`set_cullable(false)` is called automatically for them), since they affect the entire scene regardless of position.

```cpp
// From Light::set_type():
set_cullable(type_ != LIGHT_TYPE_DIRECTIONAL);
```

### Range-Based Culling

Each renderable only considers lights within range. The compositor checks whether a renderable's bounding box intersects the light's sphere of influence. Lights beyond range are excluded from the per-renderable light list.

### Performance Limits

- **Maximum lights per renderable**: 8 on desktop, 4 on PSP, 2 on Dreamcast. This is a hard limit set at compile time in `material_constants.h`
- **More lights = more uniform updates**: Each light requires several uniform uploads per renderable in GL2x. Keep the number of active lights reasonable
- **Directional lights are cheapest**: They have no range checks, no attenuation, and always sort first. A single directional light is cheaper than many point lights

### Tips for Performance

1. **Use the fewest lights necessary**. Combine nearby point lights where possible
2. **Keep ranges tight**. A light with a huge range will be considered for more renderables
3. **Use one directional light for sunlight** instead of many point lights to simulate daylight
4. **Disable lighting on materials that do not need it** with `material->set_lighting_enabled(false)`
5. **Parent moving lights to actors** so the engine can cull them together with their parent

---

## 10. Lighting Best Practices

### 1. Always Set Ambient Light

Without ambient light, surfaces not directly facing a light will be completely black. Set a low ambient value:

```cpp
lighting->set_ambient_light(smlt::Color(0.15f, 0.15f, 0.15f, 1.0f));
```

### 2. Use Directional Light for Sunlight

A single `DirectionalLight` is more efficient and more realistic for outdoor scenes than scattering many point lights:

```cpp
auto sun = create_child<smlt::DirectionalLight>(
    smlt::Params()
        .set("direction", smlt::Vec3(1, -1, 0.5))
        .set("color", smlt::Color(1, 0.95f, 0.8f, 1))
);
```

### 3. Tune Intensity to Your Scene Scale

There is no single "correct" intensity value. It depends on your scene's unit scale and material colors. Start with values in the hundreds to thousands and adjust visually:

```cpp
light->set_intensity(1000.0f);  // Good starting point for a point light
```

### 4. Disable Lighting on Self-Illuminated Objects

For objects that glow (screens, lava, magic effects), disable lighting so they appear the same regardless of light placement:

```cpp
material->set_lighting_enabled(false);
```

### 5. Parent Lights to What They Illuminate

If a light logically belongs to an object (a lantern, a muzzle flash, a headlight), parent it:

```cpp
lantern_light->set_parent(lantern_actor);
lantern_light->transform->set_position(0, 1, 0);
```

This keeps your scene graph organized and ensures culling works correctly.

### 6. Be Aware of the Center-Point Sorting Limitation

The compositor sorts lights by distance to the renderable's center. For very large meshes, lights close to the edge may be excluded even though they should illuminate nearby polygons. If you encounter this, consider splitting the mesh or adjusting light ranges.

---

## 11. Complete Examples

### Example 1: Single Point Light in a Room

A minimal scene with one point light illuminating a textured cube.

```cpp
#include "simulant/simulant.h"

using namespace smlt;

class RoomScene : public Scene {
public:
    RoomScene(Window* window) : Scene(window) {}

    void on_load() override {
        // Ambient light
        lighting->set_ambient_light(Color(0.1f, 0.1f, 0.1f, 1.0f));

        // Camera
        camera_ = create_child<Camera3D>();
        camera_->set_perspective_projection(
            Degrees(60.0f),
            float(window->width()) / float(window->height()),
            0.1f, 100.0f
        );
        camera_->transform->set_position(0, 3, -8);
        camera_->transform->look_at(0, 1, 0);

        // Render pipeline
        auto pipeline = compositor->create_layer(this, camera_)
            ->set_clear_flags(BUFFER_CLEAR_ALL)
            ->set_viewport(Viewport(VIEWPORT_TYPE_FULL, Color(0.05f, 0.05f, 0.05f, 1)));
        link_pipeline(pipeline);

        // Textured cube
        auto crate_tex = app->shared_assets->load_texture("assets/crate.png");
        auto mat = assets->create_material();
        mat->set_base_color_map(crate_tex);

        auto mesh = assets->create_mesh(VertexSpecification::DEFAULT);
        mesh->create_submesh_as_cube("cube", mat, 2.0f);
        auto actor = create_child<Actor>(mesh);
        actor->transform->set_position(0, 1, 0);

        // Single warm point light
        auto lamp = create_child<PointLight>();
        lamp->transform->set_position(0, 5, 0);
        lamp->set_color(Color(1, 0.7f, 0.3f, 1));
        lamp->set_intensity(800.0f);
        lamp->set_range(15.0f);
    }

private:
    CameraPtr camera_;
};

class RoomApp : public Application {
public:
    RoomApp(const AppConfig& config) : Application(config) {}
private:
    bool init() override {
        scenes->register_scene<RoomScene>("room");
        scenes->activate("room");
        return true;
    }
};

int main(int argc, char* argv[]) {
    AppConfig config;
    config.title = "Single Point Light";
    config.width = 1280;
    config.height = 720;
    config.fullscreen = false;

    RoomApp app(config);
    return app.run();
}
```

### Example 2: Multiple Colored Lights

A scene with three colored lights orbiting a sphere, demonstrating multi-light accumulation.

```cpp
#include "simulant/simulant.h"

using namespace smlt;

class MultiLightScene : public Scene {
public:
    MultiLightScene(Window* window) : Scene(window) {}

    void on_load() override {
        lighting->set_ambient_light(Color(0.05f, 0.05f, 0.05f, 1.0f));

        // Camera
        camera_ = create_child<Camera3D>();
        camera_->set_perspective_projection(
            Degrees(45.0f),
            float(window->width()) / float(window->height()),
            0.1f, 100.0f
        );
        camera_->transform->set_position(0, 0, -10);
        link_pipeline(compositor->create_layer(this, camera_)
            ->set_clear_flags(BUFFER_CLEAR_ALL));

        // Central sphere with a neutral material
        auto mat = assets->create_material();
        auto mesh = assets->create_mesh(VertexSpecification::DEFAULT);
        mesh->create_submesh_as_sphere("sphere", mat, 1.5f);
        sphere_ = create_child<Actor>(mesh);

        // Red light
        red_light_ = create_child<PointLight>();
        red_light_->set_color(Color(1, 0, 0, 1));
        red_light_->set_intensity(600.0f);
        red_light_->set_range(12.0f);

        // Green light
        green_light_ = create_child<PointLight>();
        green_light_->set_color(Color(0, 1, 0, 1));
        green_light_->set_intensity(600.0f);
        green_light_->set_range(12.0f);

        // Blue light
        blue_light_ = create_child<PointLight>();
        blue_light_->set_color(Color(0, 0, 1, 1));
        blue_light_->set_intensity(600.0f);
        blue_light_->set_range(12.0f);
    }

    void on_update(float dt) override {
        Scene::on_update(dt);
        time_ += dt;

        // Orbit the three lights around the sphere
        red_light_->transform->set_position(
            Vec3(cos(time_) * 5, 2, sin(time_) * 5));
        green_light_->transform->set_position(
            Vec3(cos(time_ + 2.094) * 5, 2, sin(time_ + 2.094) * 5));  // 120 degrees offset
        blue_light_->transform->set_position(
            Vec3(cos(time_ + 4.189) * 5, 2, sin(time_ + 4.189) * 5));  // 240 degrees offset

        // Slowly rotate the sphere for visual interest
        sphere_->transform->set_rotation(Vec3(0, time_ * 20, 0));
    }

private:
    CameraPtr camera_;
    ActorPtr sphere_;
    PointLightPtr red_light_;
    PointLightPtr green_light_;
    PointLightPtr blue_light_;
    float time_ = 0.0f;
};
```

### Example 3: Directional Sunlight with Fill Light

An outdoor-style scene with a directional sun and a softer fill light to reduce shadow harshness.

```cpp
#include "simulant/simulant.h"

using namespace smlt;

class OutdoorScene : public Scene {
public:
    OutdoorScene(Window* window) : Scene(window) {}

    void on_load() override {
        // Subtle ambient so shadows are not pure black
        lighting->set_ambient_light(Color(0.2f, 0.25f, 0.3f, 1.0f));

        // Camera
        camera_ = create_child<Camera3D>();
        camera_->set_perspective_projection(
            Degrees(60.0f),
            float(window->width()) / float(window->height()),
            0.1f, 500.0f
        );
        camera_->transform->set_position(0, 8, -20);
        camera_->transform->look_at(0, 2, 0);
        link_pipeline(compositor->create_layer(this, camera_)
            ->set_clear_flags(BUFFER_CLEAR_ALL)
            ->set_viewport(Viewport(VIEWPORT_TYPE_FULL, Color(0.4f, 0.6f, 0.9f, 1))));

        // Ground plane
        auto ground_mat = assets->create_material();
        auto ground_mesh = assets->create_mesh(VertexSpecification::DEFAULT);
        ground_mesh->create_submesh_as_box("ground", ground_mat, 100, 0.5, 100);
        auto ground = create_child<Actor>(ground_mesh);
        ground->transform->set_position(0, -0.25f, 0);

        // Several cubes as "buildings"
        for (int i = 0; i < 5; ++i) {
            auto cube_mat = assets->create_material();
            auto cube_mesh = assets->create_mesh(VertexSpecification::DEFAULT);
            cube_mesh->create_submesh_as_cube("cube", cube_mat, 2.0f + i * 0.5f);
            auto cube = create_child<Actor>(cube_mesh);
            cube->transform->set_position(-8 + i * 4, 1 + i * 0.25f, 5);
        }

        // Directional sunlight (warm, from upper-right)
        auto sun = create_child<DirectionalLight>(
            Params()
                .set("direction", Vec3(1, -1.5f, 0.5f))
                .set("color", Color(1, 0.9f, 0.7f, 1))
        );

        // Soft blue fill light from the opposite side
        auto fill = create_child<PointLight>();
        fill->transform->set_position(-10, 5, -5);
        fill->set_color(Color(0.4f, 0.5f, 0.8f, 1));
        fill->set_intensity(300.0f);
        fill->set_range(50.0f);
    }
};
```

---

## API Quick Reference

### Light (Base Class)

| Method | Description |
|--------|-------------|
| `set_color(Color)` | Set the light color |
| `color()` | Get the light color |
| `set_intensity(float)` | Set the intensity multiplier |
| `intensity()` | Get the intensity |
| `set_range(float)` | Set the maximum range (point lights) |
| `range()` | Get the range |
| `set_direction(Vec3)` | Set direction (also sets type to directional) |
| `direction()` | Get the direction vector |
| `light_type()` | Get the `LightType` enum value |

### PointLight

| Param | Type | Default | Description |
|-------|------|---------|-------------|
| `position` | `Vec3` | `(0, 0, 0)` | World position of the light |
| `color` | `Color` | `white` | Light color (inherited from base) |

### DirectionalLight

| Param | Type | Default | Description |
|-------|------|---------|-------------|
| `direction` | `Vec3` | `(1, -0.5, 0)` | Direction the light is pointing |
| `color` | `Color` | `white` | Light color (inherited from base) |

### LightingSettings (via `lighting` on Scene)

| Method | Description |
|--------|-------------|
| `set_ambient_light(Color)` | Set the scene's ambient light color |
| `ambient_light()` | Get the ambient light color |

### GL2x Shader Uniforms

| Uniform | Type | Description |
|---------|------|-------------|
| `s_light_position` / `s_light_position[N]` | `vec4` | Light position/direction. `w=0` directional, `w=1` point |
| `s_light_color` / `s_light_color[N]` | `vec4` | Light color |
| `s_light_intensity` / `s_light_intensity[N]` | `float` | Light intensity |
| `s_light_range` / `s_light_range[N]` | `float` | Light range |

---

## Related Documentation

- [Materials](materials.md) -- How materials interact with lighting, including `s_lighting_enabled`
- [Actors](../core-concepts/actors.md) -- Actors are the primary renderables that receive light
- [Render Pipelines](pipelines.md) -- How the compositor collects lights and builds the render queue
- [Stage Nodes](stage_nodes.md) -- Lights are StageNodes; see how the scene graph hierarchy works
