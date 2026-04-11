# Particle Systems

Particle systems in Simulant let you create dynamic visual effects such as fire, smoke, explosions, magic spells, weather, and engine trails. They are driven by data-defined scripts (`.kglp` files) and rendered as camera-facing billboards.

**Related documentation:** [Stage Nodes](../core-concepts/stage-nodes.md), [Materials](materials.md), [Particle Script Format](../assets/particle-script.md).

---

## 1. Overview

A particle system in Simulant consists of three conceptual layers:

1. **Particle Script** (`.kglp`) -- A JSON file that defines emitters, manipulators, particle quotas, dimensions, and the material used for rendering.
2. **ParticleSystem** (StageNode) -- A scene node that loads a particle script, simulates particles each frame, and submits billboards to the render queue.
3. **Particle** (struct) -- An individual particle with position, velocity, color, dimensions, and a time-to-live (TTL).

The simulation loop each frame works like this:

1. Dead particles (TTL <= 0) are swapped to the end of the active list.
2. **Manipulators** run over all living particles, modifying their color, size, velocity, or position.
3. **Emitters** spawn new particles up to the remaining quota.
4. Living particles are rebuilt into vertex data as camera-facing billboards and submitted for rendering.

```
Stage (root)
  |-- Actor "Player"
  |     |-- ParticleSystem "ExhaustTrail"
  |     `-- ParticleSystem "FootstepDust"
  |-- Stage "Environment"
  |     |-- ParticleSystem "Campfire"
  |     `-- ParticleSystem "Rain"
  `-- ParticleSystem "Explosion"
```

### Key Characteristics

- Particles are rendered as **billboarded quads** (two triangles forming a quad that always faces the camera).
- Each particle system has a **quota** -- the maximum number of simultaneous live particles.
- Emitters can be **point** or **box** shaped.
- Particles can exist in **world space** (positions are absolute) or **local space** (positions are relative to the ParticleSystem node's transform).
- A particle system can have up to **8 emitters** (`ParticleScript::MAX_EMITTER_COUNT`).

---

## 2. Particle Scripts (.kglp / .spart)

Particle scripts are JSON files with the `.kglp` (or `.spart`) extension. They describe the complete configuration of a particle effect. The loader recognizes both extensions:

```cpp
// From LoaderType::supports()
return filename.ext() == ".spart" || filename.ext() == ".kglp";
```

### Root-Level Properties

| Property | Type | Description |
|----------|------|-------------|
| `name` | string | Human-readable name for the effect |
| `quota` | integer | Maximum number of simultaneous live particles |
| `particle_width` | float | Width of each particle billboard in world units |
| `particle_height` | float | Height of each particle billboard in world units |
| `cull_each` | boolean | If `true`, each particle is individually culled (not yet implemented) |
| `material` | string | Path to a material file, or a built-in material name (e.g. `"TEXTURED_PARTICLE"`, `"TEXTURE_ONLY"`, `"DIFFUSE_ONLY"`) |
| `material.<property>` | varies | Override individual material properties (see below) |
| `emitters` | array | List of emitter definitions |
| `manipulators` | array | List of manipulator (affector) definitions |

### Material Properties in Scripts

You can set material properties directly from the particle script using the `material.` prefix. This is convenient because particle effects almost always need specific blending and texture settings.

```json
{
    "name": "fire",
    "quota": 50,
    "particle_width": 2.0,
    "particle_height": 2.0,
    "material": "TEXTURE_ONLY",
    "material.s_base_color_map": "flare.tga",
    "material.s_depth_write_enabled": false,
    "material.s_blend_func": "add"
}
```

Supported material property types that can be set from JSON:

- **bool** -- `true` or `false`
- **float** -- numeric value
- **int** -- integer value (including blend function enums like `"add"`)
- **Texture** -- path to a texture file (resolved relative to the particle script's directory)

### Built-in Particle Effects

Simulant ships with a built-in fire effect you can reference directly:

```cpp
// ParticleScript::BuiltIns::FIRE = "particles/fire.kglp"
auto fire_script = scene->assets->load_particle_script(
    ParticleScript::BuiltIns::FIRE
);
```

---

## 3. Loading Particle Scripts

Particle scripts are loaded through the `AssetManager`:

```cpp
// Load by path (relative to asset search paths)
ParticleScriptPtr script = scene->assets->load_particle_script("particles/fire.kglp");

// Load the built-in fire effect
ParticleScriptPtr fire = scene->assets->load_particle_script(
    ParticleScript::BuiltIns::FIRE
);
```

The loader parses the JSON, creates `Emitter` and `Manipulator` objects, configures the material, and returns a shared pointer to the `ParticleScript` asset.

---

## 4. Creating a ParticleSystem Node

The `ParticleSystem` is a `StageNode` subclass. Create it as a child of any node in your scene graph:

```cpp
// Create using a loaded script
auto fire = scene->assets->load_particle_script("particles/fire.kglp");
auto fire_system = create_child<ParticleSystem>(fire);

// The script is passed as a "script" parameter internally.
// You can also use Params explicitly:
Params params;
params.set<ParticleScriptRef>("script", fire);
auto fire_system2 = create_child<ParticleSystem>(params);
```

Because `ParticleSystem` is a `StageNode`, it inherits all standard node behavior:

```cpp
// Position it in the world
fire_system->transform->set_position(Vec3(0, 0, 0));

// Attach it to a moving object (e.g., a spaceship exhaust)
fire_system->set_parent(spaceship_actor);

// Hide/show it
fire_system->set_visible(false);

// Give it a name for lookup
auto trail = create_child<ParticleSystem>(trail_script)->set_name_and_get("EngineTrail");
```

### Update When Hidden

By default, hidden particle systems stop simulating. You can override this so particles continue to simulate even when the node is invisible:

```cpp
fire_system->set_update_when_hidden(true);
// Particles will keep simulating even if set_visible(false) is called
```

### Destroy on Completion

You can configure the system to automatically destroy itself when all emitters have finished and all particles have died:

```cpp
explosion_system->set_destroy_on_completion(true);
// The node will call destroy() on itself when no particles remain
// and no emitters are active
```

---

## 5. Emitters

Emitters are responsible for spawning particles. Each particle script can have up to 8 emitters.

### Emitter Types

| Type | Description |
|------|-------------|
| `point` | All particles originate from a single point |
| `box` | Particles spawn randomly within a 3D box defined by `width`, `height`, `depth` |

### Emitter Properties

| Property | Type | Description |
|----------|------|-------------|
| `type` | string | `"point"` or `"box"` (default: `"point"`) |
| `direction` | string | Space-separated `"X Y Z"` direction vector (default: `"0 1 0"`) |
| `velocity` | float | Fixed emission speed |
| `velocity_min` | float | Minimum emission speed (overrides `velocity` if both present) |
| `velocity_max` | float | Maximum emission speed |
| `ttl` | float | Fixed lifetime in seconds |
| `ttl_min` | float | Minimum particle lifetime |
| `ttl_max` | float | Maximum particle lifetime |
| `angle` | float | Cone angle in degrees from direction (0 = straight, 360 = sphere) |
| `color` | string | Single color as `"R G B A"` |
| `colors` | array | List of colors; one is chosen randomly per particle |
| `emission_rate` | float | Particles per second |
| `duration` | float | How long the emitter runs (0 = forever) |
| `repeat_delay` | float | Delay before the emitter restarts after its duration ends |
| `width` | float | Box emitter width (X dimension) |
| `height` | float | Box emitter height (Y dimension) |
| `depth` | float | Box emitter depth (Z dimension) |

### Example: Point Emitter (Flame)

```json
{
    "emitters": [{
        "type": "point",
        "direction": "0 1 0",
        "velocity_min": 2.0,
        "velocity_max": 5.0,
        "ttl_min": 0.5,
        "ttl_max": 1.5,
        "angle": 15,
        "color": "1.0 0.5 0.0 1.0",
        "emission_rate": 30
    }]
}
```

### Example: Box Emitter (Snow)

```json
{
    "emitters": [{
        "type": "box",
        "width": 100,
        "height": 5,
        "depth": 100,
        "direction": "0 -1 0",
        "velocity_min": 1.0,
        "velocity_max": 3.0,
        "ttl_min": 3.0,
        "ttl_max": 5.0,
        "angle": 10,
        "color": "1 1 1 1",
        "emission_rate": 100
    }]
}
```

### Example: Burst Emitter (Explosion)

An emitter with a short `duration` and no `repeat_delay` fires once then stops. Combine two opposing emitters for a bi-directional burst:

```json
{
    "emitters": [
        {
            "type": "point",
            "direction": "0 1 0",
            "velocity": 6,
            "ttl_min": 0.1,
            "ttl_max": 1.0,
            "angle": 180,
            "color": "1 1 1 1",
            "emission_rate": 500,
            "duration": 0.1
        },
        {
            "type": "point",
            "direction": "0 -1 0",
            "velocity": 6,
            "ttl_min": 0.1,
            "ttl_max": 1.0,
            "angle": 180,
            "color": "1 1 1 1",
            "emission_rate": 500,
            "duration": 0.1
        }
    ]
}
```

### Emission Angle

The `angle` property controls particle spread:

- **`angle: 0`** -- All particles travel exactly along the direction vector.
- **`angle: 15`** -- Particles deviate up to 15 degrees from the direction (narrow cone).
- **`angle: 90`** -- Particles spread in a wide hemisphere.
- **`angle: 180`** -- Particles spread in a full sphere around the emitter.
- **`angle: 360`** -- Special case: each particle gets a completely random direction.

### Repeat Delay

When `duration` is set and `repeat_delay` is greater than zero, the emitter cycles between active and inactive periods:

```json
{
    "duration": 0.5,
    "repeat_delay": 2.0
}
```

This creates a burst pattern: the emitter fires for 0.5 seconds, pauses for 2 seconds, then fires again.

### Activating and Deactivating Emitters at Runtime

You can toggle all emitters on a particle system:

```cpp
particle_system->set_emitters_active(false);  // Stop spawning
particle_system->set_emitters_active(true);   // Resume spawning
```

Check if any emitter is currently active:

```cpp
if (particle_system->has_active_emitters()) {
    // Emitters are still spawning particles
}
```

---

## 6. Particle Parameters

Each individual `Particle` struct holds the following data:

| Field | Type | Description |
|-------|------|-------------|
| `position` | `Vec3` | Current position (relative to emitter origin) |
| `velocity` | `Vec3` | Current velocity vector |
| `dimensions` | `Vec2` | Current width and height of the billboard |
| `initial_dimensions` | `Vec2` | Original size at spawn (used by size manipulators) |
| `ttl` | float | Time remaining until the particle dies |
| `lifetime` | float | Total lifetime (set at spawn, does not change) |
| `color` | `Color` | Current RGBA color |
| `emitter_index` | uint8_t | Index of the emitter that spawned this particle |

### How Particles Move

Each frame during `on_update()`:

```cpp
particle.position += particle.velocity * dt;
particle.ttl -= dt;

if (particle.ttl <= 0) {
    // Particle dies -- swapped to end of array
}
```

Manipulators can then modify `position`, `velocity`, `color`, and `dimensions` before the next frame.

### Color Selection at Spawn

When an emitter has multiple colors, one is chosen randomly for each spawned particle:

```json
{
    "colors": [
        "1.0 0.8 0.0 1.0",
        "1.0 0.4 0.0 1.0",
        "0.8 0.1 0.0 1.0"
    ]
}
```

### Size at Spawn

Particle dimensions are initialized from the script's `particle_width` and `particle_height`, scaled by the ParticleSystem node's transform scale:

```cpp
p.initial_dimensions = p.dimensions = Vec2(
    script->particle_width() * scale.x,
    script->particle_height() * scale.y
);
```

---

## 7. Manipulators

Manipulators (also called affectors in the file format) modify all living particles each frame. They run **after** particles move and **before** new particles are emitted.

Available manipulator types:

### Size Manipulator

Changes particle dimensions over their lifetime. Supports linear and bell curves.

**Linear rate** -- shrinks or grows particles at a constant rate:

```json
{
    "type": "size",
    "rate": -0.9
}
```

A negative rate shrinks particles; a positive rate grows them.

**Bell curve** -- particles grow to a peak size then shrink:

```json
{
    "type": "size",
    "curve": "bell(0.5, 0.2)"
}
```

The parameters are `(peak, deviation)` where `peak` is the normalized lifetime at which the particle reaches maximum size.

**Linear curve** -- explicit linear specification:

```json
{
    "type": "size",
    "curve": "linear(-0.5)"
}
```

### Color Fader

Transitions particle colors over their lifetime through a sequence of colors:

```json
{
    "type": "color_fader",
    "colors": ["1 1 1 1", "1 0.5 0 1", "0.5 0 0 1"],
    "interpolate": true
}
```

With `interpolate: true`, colors blend smoothly between keyframes. With `false`, colors snap abruptly.

The color progression is based on each particle's normalized lifetime (0 = just born, 1 = about to die).

### Alpha Fader

Transitions only the alpha channel over lifetime. Useful for fade-in/fade-out effects:

```json
{
    "type": "alpha_fader",
    "alphas": [0.0, 1.0, 1.0, 0.0],
    "interpolate": true
}
```

This makes particles fade in, stay visible, then fade out before dying.

### Direction Manipulator

Applies a constant force to every particle's position each frame:

```json
{
    "type": "direction",
    "force": "0 -9.8 0"
}
```

This is commonly used for **gravity** (downward force) or **wind** (horizontal force). The force is added to the particle's position scaled by `dt`, effectively acting as a velocity offset.

### Direction Noise Random

Applies a force plus random noise to each particle's position. Creates chaotic, organic movement:

```json
{
    "type": "direction_noise_random",
    "force": "0 -1 0",
    "noise_amount": "0.5 0.5 0.5"
}
```

- `force` -- base directional force applied uniformly
- `noise_amount` -- per-axis random noise multiplier

This is ideal for effects like swirling smoke, turbulent flames, or scattered debris.

### Adding Manipulators Programmatically

You can add manipulators to a script at runtime:

```cpp
auto script = scene->assets->load_particle_script("particles/base.kglp");

// Add gravity
script->add_manipulator(std::make_shared<DirectionManipulator>(
    script.get(), Vec3(0, -9.8f, 0)
));

// Add a color fader
std::vector<Color> fade_colors = {
    Color(1, 1, 1, 1),
    Color(1, 0.5f, 0, 1),
    Color(0.3f, 0, 0, 1)
};
script->add_manipulator(std::make_shared<ColorFader>(
    script.get(), fade_colors, true
));
```

Clear all manipulators:

```cpp
script->clear_manipulators();
```

---

## 8. World Space vs Local Space

Particle systems can operate in two coordinate spaces, controlled by `set_space()`:

### World Space (Default)

```cpp
particle_system->set_space(PARTICLE_SYSTEM_SPACE_WORLD);
```

In world space:
- Particle positions are absolute in world coordinates.
- When the ParticleSystem node moves, **existing particles stay where they are**.
- New particles spawn at the emitter's current world position.
- Ideal for effects like rain, snow, or area effects that should not move with their parent.

```
// World space: move the system up
system->transform->set_position(Vec3(0, 100, 0));

// Existing particles stay at their original world positions.
// New particles spawn 100 units higher.
```

### Local Space

```cpp
particle_system->set_space(PARTICLE_SYSTEM_SPACE_LOCAL);
```

In local space:
- Particle positions are relative to the ParticleSystem node's transform.
- When the node moves, **all particles move with it**.
- Ideal for effects attached to moving objects (engine trails, dust behind a character).

```
// Local space: move the system up
system->set_space(PARTICLE_SYSTEM_SPACE_LOCAL);
system->transform->set_position(Vec3(0, 100, 0));

// All particles move up with the system.
// Their relative positions to the emitter stay the same.
```

### Attaching to Moving Objects

For effects that follow a moving object (like a spaceship exhaust), use local space:

```cpp
auto trail_script = scene->assets->load_particle_script("particles/trail.kglp");
auto trail = spaceship->create_child<ParticleSystem>(trail_script);
trail->set_space(PARTICLE_SYSTEM_SPACE_LOCAL);

// Now when the spaceship moves and rotates, the trail follows correctly
// and the emission velocity stays aligned with the spaceship's orientation.
```

---

## 9. GPU Particle Rendering

Simulant's particle system uses a **CPU-simulated, GPU-rendered** approach:

### How It Works

1. **CPU simulation** -- Particle positions, velocities, and lifetimes are updated on the CPU each frame.
2. **Billboard reconstruction** -- Each frame, vertex data is rebuilt. Each particle becomes a quad (4 vertices) that faces the camera using the camera's up and right vectors.
3. **GPU rendering** -- The quads are submitted to the render queue as a `TRIANGLE_STRIP` arrangement with the script's material.

### Vertex Layout

Each particle billboard uses these vertex attributes:

| Attribute | Format | Description |
|-----------|--------|-------------|
| Position | 3F | World-space corner position of the quad |
| TexCoord0 | 2F | UV coordinates (0,0 to 1,1) |
| Color | 4F (or 4UB_BGRA on Dreamcast) | Per-particle RGBA color |

### Rendering Pipeline

```
ParticleSystem::do_generate_renderables()
  |
  +-> rebuild_vertex_data(camera_up, camera_right)
  |     For each particle:
  |       - Calculate world position (respecting space mode)
  |       - Build 4 corners of billboard facing camera
  |       - Write position, color, UV data
  |
  +-> Create Renderable with TRIANGLE_STRIP arrangement
  +-> Submit to render queue with the script's material
```

### Material Considerations

Particle effects almost always need:
- **Additive or alpha blending** -- so particles composite nicely over the scene.
- **Depth write disabled** -- so particles do not occlude each other incorrectly.
- **A texture** -- a soft circular sprite, spark, or flare texture.

```json
{
    "material": "TEXTURE_ONLY",
    "material.s_base_color_map": "flare.tga",
    "material.s_depth_write_enabled": false,
    "material.s_blend_func": "add"
}
```

Common blend functions for particles:
- `"add"` -- Additive blending (bright, glowing effects like fire and magic).
- `"alpha"` -- Alpha blending (smoke, fog, transparent effects).

---

## 10. Creating Particles Programmatically

While the primary workflow uses `.kglp` scripts, you can also construct particle systems entirely in code by manipulating the `ParticleScript` object directly.

### Creating a Script from Scratch

```cpp
// Create a new material for the particles
auto material = stage->assets->new_material();
material->set_diffuse_map(flare_texture);
material->set_blend_func(BLEND_FUNC_ADD);
material->set_depth_write_enabled(false);

// Create the particle script
auto script = std::make_shared<ParticleScript>(asset_id, asset_manager);
script->set_name("CustomEffect");
script->set_quota(200);
script->set_particle_width(1.5f);
script->set_particle_height(1.5f);
script->set_material(material);

// Create an emitter
Emitter emitter;
emitter.type = PARTICLE_EMITTER_POINT;
emitter.direction = Vec3(0, 1, 0);
emitter.velocity_range = {3.0f, 6.0f};
emitter.ttl_range = {0.5f, 2.0f};
emitter.angle = Degrees(20);
emitter.colors = {Color::white()};
emitter.emission_rate = 50;

script->push_emitter(emitter);

// Add a size manipulator
auto size_manip = std::make_shared<SizeManipulator>(script.get());
size_manip->set_linear_curve(-0.5f);
script->add_manipulator(size_manip);

// Create the ParticleSystem node
auto system = create_child<ParticleSystem>(script);
```

### Modifying an Existing Script

```cpp
auto script = scene->assets->load_particle_script("particles/fire.kglp");

// Change the emission rate
auto* emitter = script->mutable_emitter(0);
emitter->emission_rate = 100;

// Change the color palette
emitter->colors = {
    Color(1, 0.2f, 0, 1),
    Color(1, 0.6f, 0, 1),
    Color(1, 1, 0.5f, 1)
};

// Clear and replace manipulators
script->clear_manipulators();
script->add_manipulator(std::make_shared<AlphaFader>(
    script.get(), std::vector<float>{0, 1, 0.8f, 0}, true
));
```

### Querying Particle State at Runtime

```cpp
// How many particles are currently alive?
std::size_t count = particle_system->particle_count();

// Access individual particles
for (std::size_t i = 0; i < particle_system->particle_count(); ++i) {
    const Particle& p = particle_system->particle(i);
    // p.position, p.velocity, p.color, p.ttl, etc.
}
```

---

## 11. Performance Considerations

Particle systems can become expensive quickly. Here is how to keep them performant:

### Quota Management

The `quota` is the single most important performance setting. It caps the maximum number of simultaneous particles:

```json
{
    "quota": 50
}
```

- **Low quota** (10-100): Small effects like muzzle flashes, sparks.
- **Medium quota** (100-500): Campfires, fountains, dust clouds.
- **High quota** (500-2000): Explosions, heavy rain, dense smoke.

Each particle adds 4 vertices and a vertex range to the vertex buffer each frame.

### Emission Rate vs Quota

If `emission_rate` is too high relative to `quota`, particles will die quickly and the effect looks sparse. If it is too low, the effect looks thin. Balance them:

```
quota / ttl_average >= emission_rate * duration
```

For a continuously running emitter:
```
quota >= emission_rate * ttl_average
```

### Vertex Rebuild

Vertex data is rebuilt **every frame** for every particle system. This means:
- Each particle = 4 vertices written to the vertex buffer.
- 200 particles = 800 vertices per frame per system.
- Multiple systems multiply this cost.

Keep quotas as low as possible for the desired effect.

### Update When Hidden

By default, hidden particle systems skip all simulation. Only enable `update_when_hidden` if you truly need particles to keep simulating while offscreen:

```cpp
particle_system->set_update_when_hidden(false);  // Default, most efficient
```

### Destroy on Completion

For one-shot effects (explosions, impacts), enable `destroy_on_completion` so the system cleans itself up:

```cpp
particle_system->set_destroy_on_completion(true);
```

### Material Sharing

All particles in a system share a single material. This is efficient because the renderer can batch all particles together into one draw call.

### Per-Particle Culling

The `cull_each` property would enable individual particle frustum culling, but this is **not yet implemented**. Currently, the entire system is culled as one unit based on its AABB.

---

## 12. Common Effects

### Fire

A narrow upward cone with shrinking particles and warm colors:

```json
{
    "name": "fire",
    "quota": 50,
    "particle_width": 2.0,
    "particle_height": 2.0,
    "material": "TEXTURE_ONLY",
    "material.s_base_color_map": "flare.tga",
    "material.s_depth_write_enabled": false,
    "material.s_blend_func": "add",

    "emitters": [{
        "type": "point",
        "angle": 15,
        "emission_rate": 25,
        "ttl_min": 1.0,
        "ttl_max": 1.0,
        "direction": "0 1 0",
        "velocity_min": 2.0,
        "velocity_max": 5.0,
        "color": "0.3 0.05 0 1.0"
    }],

    "manipulators": [{
        "type": "size",
        "rate": -0.9
    }]
}
```

### Smoke

Wide spread, slow rising, with alpha fade-out:

```json
{
    "name": "smoke",
    "quota": 80,
    "particle_width": 3.0,
    "particle_height": 3.0,
    "material": "TEXTURE_ONLY",
    "material.s_base_color_map": "smoke.tga",
    "material.s_depth_write_enabled": false,
    "material.s_blend_func": "alpha",

    "emitters": [{
        "type": "point",
        "angle": 45,
        "emission_rate": 10,
        "ttl_min": 2.0,
        "ttl_max": 4.0,
        "direction": "0 1 0",
        "velocity_min": 0.5,
        "velocity_max": 1.5,
        "color": "0.4 0.4 0.4 0.5"
    }],

    "manipulators": [
        {
            "type": "size",
            "rate": 0.5
        },
        {
            "type": "alpha_fader",
            "alphas": [0.0, 0.6, 0.4, 0.0],
            "interpolate": true
        }
    ]
}
```

### Explosion

High-velocity burst in all directions, short-lived:

```json
{
    "name": "explosion",
    "quota": 200,
    "particle_width": 1.5,
    "particle_height": 1.5,
    "material": "TEXTURE_ONLY",
    "material.s_base_color_map": "flare.tga",
    "material.s_depth_write_enabled": false,
    "material.s_blend_func": "add",

    "emitters": [
        {
            "type": "point",
            "direction": "0 1 0",
            "velocity_min": 5.0,
            "velocity_max": 15.0,
            "ttl_min": 0.2,
            "ttl_max": 0.8,
            "angle": 360,
            "color": "1 1 0.8 1",
            "emission_rate": 1000,
            "duration": 0.15
        }
    ],

    "manipulators": [
        {
            "type": "size",
            "curve": "bell(0.3, 0.15)"
        },
        {
            "type": "color_fader",
            "colors": ["1 1 1 1", "1 0.5 0 1", "0.5 0 0 1", "0 0 0 0"],
            "interpolate": true
        }
    ]
}
```

### Magic / Spell Effect

Swirling particles with noise and color cycling:

```json
{
    "name": "magic_orb",
    "quota": 100,
    "particle_width": 0.8,
    "particle_height": 0.8,
    "material": "TEXTURE_ONLY",
    "material.s_base_color_map": "spark.tga",
    "material.s_depth_write_enabled": false,
    "material.s_blend_func": "add",

    "emitters": [{
        "type": "point",
        "direction": "0 1 0",
        "velocity_min": 1.0,
        "velocity_max": 3.0,
        "ttl_min": 1.0,
        "ttl_max": 2.0,
        "angle": 90,
        "colors": ["0 0.5 1 1", "0.5 0 1 1", "1 0 0.5 1"],
        "emission_rate": 40
    }],

    "manipulators": [
        {
            "type": "direction_noise_random",
            "force": "0 0.5 0",
            "noise_amount": "1.0 1.0 1.0"
        },
        {
            "type": "color_fader",
            "colors": ["1 1 1 1", "0 0.3 1 0.5"],
            "interpolate": true
        }
    ]
}
```

### Weather: Rain

A large box emitter shooting downward at high speed:

```json
{
    "name": "rain",
    "quota": 500,
    "particle_width": 0.3,
    "particle_height": 1.5,
    "material": "TEXTURE_ONLY",
    "material.s_base_color_map": "rain_drop.tga",
    "material.s_depth_write_enabled": false,
    "material.s_blend_func": "alpha",

    "emitters": [{
        "type": "box",
        "width": 200,
        "height": 5,
        "depth": 200,
        "direction": "0 -1 0",
        "velocity_min": 15.0,
        "velocity_max": 25.0,
        "ttl_min": 0.5,
        "ttl_max": 1.0,
        "angle": 5,
        "color": "0.7 0.8 1 0.6",
        "emission_rate": 500
    }],

    "manipulators": [{
        "type": "direction",
        "force": "0.5 0 0"
    }]
}
```

### Weather: Snow

Wide box, slow falling, with random drift:

```json
{
    "name": "snow",
    "quota": 300,
    "particle_width": 0.5,
    "particle_height": 0.5,
    "material": "TEXTURE_ONLY",
    "material.s_base_color_map": "snowflake.tga",
    "material.s_depth_write_enabled": false,
    "material.s_blend_func": "alpha",

    "emitters": [{
        "type": "box",
        "width": 150,
        "height": 3,
        "depth": 150,
        "direction": "0 -1 0",
        "velocity_min": 1.0,
        "velocity_max": 3.0,
        "ttl_min": 3.0,
        "ttl_max": 6.0,
        "angle": 10,
        "color": "1 1 1 0.9",
        "emission_rate": 100
    }],

    "manipulators": [{
        "type": "direction_noise_random",
        "force": "0 -0.5 0",
        "noise_amount": "0.3 0.1 0.3"
    }]
}
```

---

## 13. Best Practices

1. **Keep quotas low.** The quota is a hard cap, but every slot costs CPU (simulation) and GPU (vertex buffer writes). Start with the smallest quota that still looks good.

2. **Use additive blending for glow effects.** Fire, magic, and explosions look best with `"add"` blending. Smoke and fog need `"alpha"` blending.

3. **Disable depth write on particles.** Set `material.s_depth_write_enabled: false` so particles do not create harsh depth boundaries between overlapping particles.

4. **Prefer fewer, larger particles over many tiny ones.** A fire effect with 50 well-sized particles often looks better than 200 microscopic ones.

5. **Use local space for attached effects.** Engine trails, spell effects on characters, and dust behind vehicles should use `PARTICLE_SYSTEM_SPACE_LOCAL` so they follow their parent naturally.

6. **Use world space for environmental effects.** Rain, snow, area fog, and stationary campfires should stay in world space.

7. **Enable `destroy_on_completion` for one-shot effects.** Explosions, impacts, and death effects should self-clean to avoid leaking nodes.

8. **Disable `update_when_hidden` by default.** There is no reason to simulate particles the player cannot see. Only enable it for effects that must persist through visibility changes.

9. **Use color faders instead of many colors.** Rather than listing 10 colors on the emitter, use a color fader manipulator with 3-4 keyframes and `interpolate: true` for smooth transitions.

10. **Preload particle scripts.** Loading a `.kglp` file involves JSON parsing. Load scripts during level loading, not during gameplay.

11. **Scale particle size through the node transform.** The particle system respects its node's `scale_factor`, so you can uniformly resize effects by scaling the parent node rather than editing the script.

12. **Combine effects with multiple emitters.** A campfire can have one emitter for flames (upward, narrow cone) and another for sparks (wider angle, longer lifetime) in the same script.

---

## 14. Complete Particle Script Example

Here is a full, production-quality particle script for a campfire effect with flames, embers, and smoke all in one file:

```json
{
    "name": "Campfire",
    "quota": 200,
    "particle_width": 2.0,
    "particle_height": 2.0,
    "cull_each": false,

    "material": "TEXTURE_ONLY",
    "material.s_base_color_map": "flare.tga",
    "material.s_depth_write_enabled": false,
    "material.s_blend_func": "add",

    "emitters": [
        {
            "type": "point",
            "direction": "0 1 0",
            "velocity_min": 2.0,
            "velocity_max": 4.0,
            "ttl_min": 0.8,
            "ttl_max": 1.5,
            "angle": 15,
            "color": "0.8 0.3 0.0 1.0",
            "emission_rate": 20
        },
        {
            "type": "point",
            "direction": "0 1 0",
            "velocity_min": 3.0,
            "velocity_max": 7.0,
            "ttl_min": 1.0,
            "ttl_max": 2.0,
            "angle": 30,
            "colors": [
                "1 0.9 0.3 1",
                "1 0.5 0 1",
                "0.6 0.1 0 1"
            ],
            "emission_rate": 8
        },
        {
            "type": "point",
            "direction": "0 1 0",
            "velocity_min": 0.5,
            "velocity_max": 1.5,
            "ttl_min": 2.0,
            "ttl_max": 4.0,
            "angle": 40,
            "color": "0.3 0.3 0.3 0.4",
            "emission_rate": 5
        }
    ],

    "manipulators": [
        {
            "type": "size",
            "rate": -0.8
        },
        {
            "type": "color_fader",
            "colors": ["1 1 1 1", "1 0.4 0 1", "0.3 0 0 0.5", "0 0 0 0"],
            "interpolate": true
        },
        {
            "type": "direction_noise_random",
            "force": "0 0.5 0",
            "noise_amount": "0.2 0.3 0.2"
        }
    ]
}
```

This script defines:

- **Emitter 0** -- Main flame column. Narrow cone, moderate speed, warm orange color.
- **Emitter 1** -- Sparks. Wider cone, faster, multiple warm colors for variety.
- **Emitter 2** -- Smoke. Slow, wide spread, grey with partial transparency.
- **Size manipulator** -- Shrinks all particles over time at a rate of -0.8 per second.
- **Color fader** -- Transitions particles from white-hot through orange to dark fade-out.
- **Direction noise random** -- Adds gentle upward drift with slight random turbulence for organic movement.

### Using It In Code

```cpp
// Load the script (ideally during level loading)
auto campfire_script = scene->assets->load_particle_script("effects/campfire.kglp");

// Create the particle system at the fire pit location
auto campfire = create_child<ParticleSystem>(campfire_script);
campfire->transform->set_position(Vec3(10, 0, 5));

// Optionally scale the entire effect
campfire->transform->set_scale(Vec3(1.5f, 1.5f, 1.5f));
```

---

## Summary

| Concept | Key Methods / Properties |
|---------|------------------------|
| Loading scripts | `scene->assets->load_particle_script(path)` |
| Creating system | `create_child<ParticleSystem>(script)` |
| World vs local space | `set_space(PARTICLE_SYSTEM_SPACE_WORLD)` / `set_space(PARTICLE_SYSTEM_SPACE_LOCAL)` |
| Toggle emitters | `set_emitters_active(true/false)` |
| Check active | `has_active_emitters()` |
| Update when hidden | `set_update_when_hidden(true/false)` |
| Auto-destroy | `set_destroy_on_completion(true/false)` |
| Particle count | `particle_count()` |
| Access particle | `particle(index)` |
| Script reference | `script()` returns `ParticleScript*` |
| Quota | `script()->set_quota(n)` |
| Add manipulator | `script()->add_manipulator(...)` |
| Clear manipulators | `script()->clear_manipulators()` |
| Emitter access | `script()->emitter(i)` / `script()->mutable_emitter(i)` |
| Built-in effect | `ParticleScript::BuiltIns::FIRE` |
