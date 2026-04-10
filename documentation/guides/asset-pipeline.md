# Asset Pipeline Guide

This guide covers the complete asset workflow for the Simulant game engine -- from preparing content in your authoring tools to loading it in your game across all target platforms.

---

## Table of Contents

1. [Overview of the Asset Workflow](#1-overview-of-the-asset-workflow)
2. [3D Model Preparation](#2-3d-model-preparation)
3. [Textures](#3-textures)
4. [Materials](#4-materials)
5. [Audio](#5-audio)
6. [Fonts](#6-fonts)
7. [Particle Scripts](#7-particle-scripts)
8. [Asset Organization](#8-asset-organization)
9. [Platform-Specific Considerations](#9-platform-specific-considerations)
10. [Build Process and Asset Embedding](#10-build-process-and-asset-embedding)
11. [Best Practices for Asset Management](#11-best-practices-for-asset-management)
12. [Common Issues and Solutions](#12-common-issues-and-solutions)

**Related documentation:**
- [Asset Managers](../assets/asset-managers.md) -- Loading and managing assets at runtime
- [Mesh Formats](../assets/mesh-formats.md) -- Supported 3D file formats and loaders
- [Materials](../rendering/materials.md) -- Material properties and shader assignment
- [Textures](../rendering/textures.md) -- Texture loading, filtering, and format conversion

---

## 1. Overview of the Asset Workflow

Simulant uses a two-phase asset workflow:

1. **Authoring Phase** -- Create content in external tools (Blender, GIMP, Audacity, etc.)
2. **Integration Phase** -- Place assets in your project and load them through Simulant's `AssetManager`

### The Asset Pipeline at a Glance

```
[Blender] -----> .glb / .obj ----+
[GIMP/Krita] --> .png / .jpg ----+--> [assets/ directory] --> [AssetManager::load_*()] --> [In-Game]
[Audacity] ----> .ogg / .wav ----+
[Text Editor] -> .smat / .kglp --+
```

### Key Principles

- **Assets are reference-counted.** When you load an asset, the `AssetManager` holds an internal reference. Your code receives another. When both are released, the asset is garbage-collected.
- **Store AssetIDs, not shared_ptrs.** Holding a `shared_ptr` prevents garbage collection. Store the ID and borrow the pointer when needed.
- **Use the right AssetManager.** `scene->assets` for scene-specific content (auto-cleaned on scene unload). `window->shared_assets` for global content like UI fonts and common sound effects.
- **Paths are relative.** Simulant's Virtual File System (VFS) searches configured paths -- never use absolute paths.

### Loading Assets in Code

```cpp
// Scene-local assets (cleaned up when scene unloads)
auto mesh = assets->load_mesh("meshes/hero.glb");
auto texture = assets->load_texture("textures/ground.png");

// Shared assets (persist across all scenes)
auto font = window->shared_assets->load_font("fonts/Orbitron.ttf");
auto click_sound = window->shared_assets->load_sound("sounds/click.wav");
```

---

## 2. 3D Model Preparation

### Supported Formats

Simulant supports the following mesh file formats out of the box:

| Format | Extension | Skeletal Animation | Notes |
|--------|-----------|-------------------|-------|
| **glTF 2.0** | `.glb`, `.gltf` | Yes (skinned + joint-based) | **Recommended format**. ASCII and binary supported. Creates prefabs automatically. |
| **Wavefront OBJ** | `.obj` | No | Simple static meshes. Widely supported by all DCC tools. |
| **Milkshape3D** | `.ms3d` | Yes | All versions supported. Generates skeleton automatically. |
| **Quake 2 MD2** | `.md2` | Yes (vertex animation) | Frame-based animation only. |
| **Quake 2 BSP** | `.bsp` (v38) | No | Level format. Processes mesh data, textures/materials (limited), and entities. |
| **X-Wing OPT** | `.opt` | No | Legacy format support. |
| **Tiled TMX** | `.tmx` | No | 2D tile map format. |

### Recommended Format: glTF 2.0

For new projects, **glTF 2.0** is the recommended format. It is the most feature-complete loader, supporting:

- Skeletal animation (both skinned meshes and joint-based)
- PBR materials (diffuse, normal, metallic, roughness maps)
- Multiple submeshes with individual materials
- Embedded textures (in `.glb` files)
- Automatic prefab creation in Simulant

### Exporting from Blender

#### glTF Export Settings

1. Install the glTF Blender exporter if not already present (built into Blender 2.80+)
2. Go to **File > Export > glTF 2.0 (.glb/.gltf)**
3. Configure the following settings:

| Setting | Value | Notes |
|---------|-------|-------|
| **Format** | glTF Binary (.glb) | Single file, recommended for games |
| **Include > Selected Objects** | Checked | Export only what you need |
| **Transform > +Y Up** | **Checked** | Critical -- Simulant expects Y-up |
| **Geometry > Normals** | Checked | Required for proper lighting |
| **Geometry > UVs** | Checked | Required for textured meshes |
| **Geometry > Tangents** | Checked (if needed) | Needed for normal maps |
| **Geometry > Colors** | Checked (if vertex colors) | Include vertex colors |
| **Animation > Animation** | Checked (if animating) | Export keyframe animations |
| **Animation > Skinning** | Checked (if skinned) | Required for skeletal animation |
| **Texture > Image Format** | JPEG or PNG | JPEG for photos, PNG for graphics with alpha |

#### OBJ Export Settings

If you need OBJ format instead:

| Setting | Value | Notes |
|---------|-------|-------|
| **Forward** | -Z Forward | Matches Simulant's default |
| **Up** | Y Up | Simulant expects Y-up |
| **Write Normals** | Checked | Required for lighting |
| **Write UVs** | Checked | Required for textures |
| **Triangulate Faces** | Checked | Simulant works best with triangles |
| **Objects as OBJ Objects** | Checked | Preserves object separation |

### Y-Up vs Z-Up Coordinate Systems

Simulant uses a **Y-up** coordinate system:
- **+Y** points up
- **+Z** points forward (into the screen by default)
- **+X** points right

Many 3D tools use **Z-up** by default (Maya, 3ds Max, Blender in some configurations). You must export with Y-up enabled.

**If your model appears rotated in-game**, this is almost certainly a coordinate system mismatch. Fix it at export time rather than applying a rotation transform -- this ensures bones, animations, and normals are all correct.

> **Note for glTF:** The Simulant glTF loader expects files exported as **Y+ UP**. If your model renders incorrectly, re-export with the +Y Up option enabled.

### Scale and Units

Simulant uses **world units** (meters by convention). Keep these guidelines in mind:

- **1 unit = 1 meter** is the recommended convention
- A character should be approximately **1.5 to 2.0 units** tall
- A room might be **10 x 10 x 3 units** (10m x 10m x 3m)
- Very large or very small scales can cause precision issues, especially on constrained platforms

If your model imports at the wrong scale, you can:
1. Scale it in your DCC tool before export (preferred)
2. Scale it in code: `actor->transform->set_scale(0.01f, 0.01f, 0.01f);`

### Materials and Textures in Models

When exporting glTF files, you can embed texture references directly in the model file. Simulant will attempt to load these textures from the paths specified.

For OBJ files, the associated `.mtl` file is processed and materials are created automatically. Texture paths in the MTL file should be relative to the OBJ file location.

**Texture path override:** If your model references `.jpg` textures but you want to load `.png` versions at runtime, use `MeshLoadOptions::override_texture_extension`:

```cpp
smlt::MeshLoadOptions opts;
opts.override_texture_extension = ".png";
auto mesh = assets->load_mesh("models/hero.obj", spec, opts);
```

### Skeleton and Animation Requirements

#### Skeletal Animation (glTF and MS3D)

For skinned skeletal animation:

- Each vertex should be weighted to **1-4 joints**
- Total weights per vertex should **sum to 1.0**
- Joint names should be meaningful (used for animation control at runtime)
- Rest pose should be the **T-pose or A-pose**

#### Keyframe Animation (MD2)

MD2 uses vertex-level animation (morphing between frames). Each frame is a complete mesh:

- Export all animation frames
- The frame rate is specified in the MD2 file header
- No skeletal data -- each frame is a complete vertex buffer

#### Animation Controller

When a glTF with animations is loaded, Simulant automatically creates an `AnimationController` on the prefab. You can play animations by name:

```cpp
auto prefab = assets->load_prefab("models/character.glb");
auto instance = prefab->instantiate(stage);

// The AnimationController is spawned automatically
// Play animations by name
// (access through the AnimationController component)
```

For MS3D files, you need to specify keyframe ranges before playing:

```cpp
auto mesh = assets->load_mesh("models/character.ms3d");
// Configure animation keyframes on the skeleton
```

> **Important:** IK (Inverse Kinematics) constraints are **not supported** in Simulant. If your animation uses IK, you must **bake IK into keyframes** before exporting.

#### LOD and Mesh Submeshes

Each submesh in a model gets its own material. You can use this to:
- Apply different materials to different parts of a model
- Swap materials at runtime using **material slots**:

```cpp
// Each submesh has 8 material slots
mesh->submesh("body")->set_material_at_slot(MATERIAL_SLOT1, alt_material);

auto actor = stage->new_actor_with_mesh(mesh);
actor->use_material_slot(MATERIAL_SLOT1);  // Uses the alternate material
```

---

## 3. Textures

### Supported Formats

Simulant supports loading textures in multiple formats:

| Format | Extension | Transparency | Notes |
|--------|-----------|-------------|-------|
| **PNG** | `.png` | Yes (alpha channel) | **Recommended**. Lossless, supports transparency. |
| **JPEG** | `.jpg`, `.jpeg` | No | Lossy compression. Good for photos and skyboxes. |
| **DDS** | `.dds` | Yes | DirectDraw Surface. Can include pre-compressed GPU formats. |
| **DTEX** | `.dtex` | Yes | Dreamcast native texture format. |
| **KMG** | `.kmg` | Yes | KallistiOS image format (Dreamcast). |
| **WAL** | `.wal` | Limited | Quake 2 wall texture format. |
| **PCX** | `.pcx` | Limited | Legacy format. |

### Size Limitations

Texture size limits vary by platform:

| Platform | Maximum Texture Size | Power of Two Required? |
|----------|--------------------|----------------------|
| **Desktop (Linux/Windows)** | Driver-dependent (typically 4096-16384) | No (NPOT supported) |
| **PSP** | **512 pixels** | **Yes** (textures must be power-of-two) |
| **Dreamcast (GL1X)** | **1024 pixels** | Recommended |

> **Critical for PSP:** All textures must be **power-of-two dimensions** (e.g., 256x256, 512x512, 1024x512). Non-power-of-two textures will fail to load or render incorrectly on PSP.

### Mipmap Generation

Mipmaps are **generated automatically by default** each time a texture is uploaded to the GPU. This improves rendering quality at distance and can reduce memory bandwidth.

```cpp
// Mipmaps are generated by default (no action needed)
auto tex = assets->load_texture("textures/ground.png");

// Disable mipmap generation
tex->set_mipmap_generation(smlt::MIPMAP_GENERATE_NONE);

// Re-enable it
tex->set_mipmap_generation(smlt::MIPMAP_GENERATE_AUTO);
```

### Filter Modes

Simulant supports three filter modes:

| Mode | Quality | Performance | Use Case |
|------|---------|-------------|----------|
| **Point** | Lowest | Best | Pixel art, retro-style games |
| **Bilinear** | Good | Good | Default choice for most textures |
| **Trilinear** | Best | Moderate | Highest quality, uses mipmaps |

```cpp
// Default is point filtering
auto tex = assets->load_texture("textures/hero.png");

// Set bilinear filtering
tex->set_texture_filter(smlt::TEXTURE_FILTER_BILINEAR);

// Or trilinear
tex->set_texture_filter(smlt::TEXTURE_FILTER_TRILINEAR);
```

### Texture Compression

For desktop platforms, you can use **DDS files** with pre-compressed GPU texture formats (DXT1, DXT3, DXT5). This reduces VRAM usage and load times.

For Dreamcast, use **DTEX** or **KMG** formats which are native to the KallistiOS toolchain.

On PSP, textures are typically loaded as raw PNG/JPG and converted to the PSP's native texture format at load time.

### Texture Types and Map Channels

Simulant materials support multiple texture map types:

| Map | Property | Purpose |
|-----|----------|---------|
| **Diffuse/Albedo** | `s_diffuse_map` | Base color of the surface |
| **Normal Map** | `s_normal_map` | Surface detail without extra geometry |
| **Specular Map** | `s_specular_map` | Specular highlight intensity per-pixel |
| **Light Map** | `s_light_map` | Pre-baked lighting data |

Each texture map can have its own **transform matrix** for UV offset and tiling:

```cpp
material->set_property_value("s_diffuse_map_matrix", transform_matrix);
material->set_property_value("s_normal_map_matrix", normal_transform);
```

### Single-Channel Textures and Format Conversion

Some loaders (like the `.fnt` font loader) load single-channel textures but require RGBA at render time. Use `Texture::convert()` to transform formats:

```cpp
// Convert a single-channel texture to RGBA by duplicating the red channel
uint8_t channels[4] = {
    TEXTURE_CHANNEL_RED,
    TEXTURE_CHANNEL_RED,
    TEXTURE_CHANNEL_RED,
    TEXTURE_CHANNEL_RED
};
texture->convert(smlt::TEXTURE_FORMAT_RGBA_4UB_8888, channels);

// You can also use TEXTURE_CHANNEL_ONE and TEXTURE_CHANNEL_ZERO
uint8_t rgba[4] = {
    TEXTURE_CHANNEL_RED,   // R from source R
    TEXTURE_CHANNEL_RED,   // G from source R
    TEXTURE_CHANNEL_RED,   // B from source R
    TEXTURE_CHANNEL_ONE    // A = 1.0 (fully opaque)
};
```

### Disabling GPU Upload

If you only need the pixel data (e.g., for generating heightmaps) and don't want to upload to the GPU:

```cpp
auto tex = assets->load_texture("textures/heightmap.png");
tex->set_auto_upload(false);
// Access tex->data() for raw pixel data
```

### Texture Data Retention

By default, texture pixel data is freed after GPU upload to save memory. Change this behavior if you need to retain CPU access:

```cpp
// Keep texture data in memory even after GPU upload
texture->set_free_data_mode(smlt::TEXTURE_FREE_DATA_NEVER);

// Default: free data after upload
texture->set_free_data_mode(smlt::TEXTURE_FREE_DATA_AFTER_UPLOAD);
```

---

## 4. Materials

### The .smat File Format

Simulant Material (`.smat`) files are **JSON documents** that define material properties and render passes. The minimal valid `.smat` file is:

```json
{
    "passes": [{}]
}
```

This creates a single-pass material with all properties set to their defaults.

### Full .smat Structure

```json
{
    "custom_properties": [
        {
            "name": "my_custom_value",
            "type": "float",
            "default": 1.0
        }
    ],
    "property_values": {
        "s_material_diffuse": "1 0 0 1",
        "s_material_ambient": "0.2 0.2 0.2 1"
    },
    "passes": [
        {
            "property_values": {
                "s_diffuse_map": "textures/brick_diffuse.png",
                "s_normal_map": "textures/brick_normal.png",
                "s_specular_map": "textures/brick_specular.png"
            },
            "vertex_shader": "shaders/brick.vert",
            "fragment_shader": "shaders/brick.frag"
        }
    ]
}
```

### Built-in Material Properties

All materials include these built-in properties (prefixed with `s_`):

| Property | Type | Description |
|----------|------|-------------|
| `s_material_ambient` | Vec4 | Ambient color |
| `s_material_diffuse` | Vec4 | Diffuse color (RGBA) |
| `s_material_specular` | Vec4 | Specular color |
| `s_material_shininess` | float | Specular shininess factor |
| `s_diffuse_map` | TexturePtr | Diffuse/albedo texture |
| `s_normal_map` | TexturePtr | Normal map texture |
| `s_specular_map` | TexturePtr | Specular map texture |
| `s_light_map` | TexturePtr | Light map texture |
| `s_diffuse_map_matrix` | Mat3 | UV transform for diffuse map |
| `s_normal_map_matrix` | Mat3 | UV transform for normal map |
| `s_blend_func` | int | Blend mode function ID |
| `s_cull_mode` | int | Face culling mode |
| `s_depth_test_enabled` | bool | Enable depth testing |
| `s_depth_write_enabled` | bool | Enable depth writing |
| `s_lighting_enabled` | bool | Enable lighting calculations |
| `s_textures_enabled` | bool | Enable texturing |
| `s_shade_model` | int | Shading model (flat or smooth) |
| `s_polygon_mode` | int | Polygon rendering mode |
| `s_point_size` | float | Point sprite size |

> **Naming convention:** Custom properties should **not** use the `s_` prefix, which is reserved for built-in properties. Property names must be valid GLSL identifier names.

### Shader Assignment (GL2X Renderer)

When using the **GL 2.x renderer** (desktop), you can assign custom vertex and fragment shaders in the pass dictionary:

```json
{
    "passes": [
        {
            "vertex_shader": "shaders/custom.vert",
            "fragment_shader": "shaders/custom.frag",
            "property_values": {
                "my_uniform_value": "42"
            }
        }
    ]
}
```

Shader paths are resolved relative to the `.smat` file, or through the VFS search paths.

> **Default behavior:** If you do not specify shaders, a default pass-through shader is used that performs basic fixed-function rendering.

For the **GL 1.x renderer** (Dreamcast), custom shaders are not supported -- rendering uses fixed-function OpenGL ES.

### Multi-Pass Materials

A material can have multiple render passes. Each pass renders the submesh independently, allowing for effects like layered textures, outlines, or custom post-process effects.

```json
{
    "passes": [
        {
            "property_values": {
                "s_diffuse_map": "textures/base_color.png"
            }
        },
        {
            "property_values": {
                "s_diffuse_map": "textures/overlay.png",
                "s_blend_func": "BLEND_ADD"
            }
        }
    ]
}
```

In code:

```cpp
auto material = assets->create_material();
material->set_pass_count(2);
material->pass(0)->set_diffuse_map(texture1);
material->pass(1)->set_diffuse_map(texture2);
```

To share a texture across all passes, set it at the material level:

```cpp
auto material = assets->create_material();
material->set_pass_count(2);
material->set_diffuse_map(shared_texture);  // Available to all passes
```

### Creating Materials in Code

```cpp
auto mat = assets->create_material();
mat->set_base_color(smlt::Color(0.8f, 0.2f, 0.2f, 1.0f));
mat->set_shininess(0.5f);
mat->set_diffuse_map(diffuse_texture);
mat->set_normal_map(normal_texture);

// Create a material from a single texture
auto simple_mat = assets->create_material_from_texture(my_texture);

// Clone an existing material
auto variant = assets->clone_material(original_mat->id());
variant->set_base_color(smlt::Color(0.2f, 0.8f, 0.2f, 1.0f));
```

### Material File Extensions

Simulant also supports `.material` file extension for material scripts. Both `.smat` and `.material` are recognized:

```cpp
auto mat1 = assets->load_material("materials/hero.smat");
auto mat2 = assets->load_material("materials/ground.material");
```

---

## 5. Audio

### Supported Formats

| Format | Extension | Streaming | Platform Support |
|--------|-----------|-----------|-----------------|
| **OGG Vorbis** | `.ogg` | Yes | Linux, Windows, Dreamcast |
| **WAV** | `.wav` | No (loaded into memory) | All platforms |

> **Note:** OGG is **not supported on PSP** due to missing audio implementation. Use WAV for PSP audio.

### Music vs Sound Effects

| Type | Format | Stream? | Reason |
|------|--------|---------|--------|
| **Background Music** | OGG | Yes | Large files, streamed from disk to save RAM |
| **Sound Effects** | WAV | No | Short files, loaded into memory for instant playback |

```cpp
// Background music - stream from disk
smlt::SoundFlags music_flags;
music_flags.stream_audio = true;  // Default
auto music = assets->load_sound("music/forest.ogg", music_flags);

// Sound effect - load fully into memory
smlt::SoundFlags sfx_flags;
sfx_flags.stream_audio = false;
auto explosion = assets->load_sound("sounds/explosion.wav", sfx_flags);
```

### Playing Sounds

All `StageNode` objects are `AudioSource`s. Play sounds through them for positional audio:

```cpp
// Positional sound (comes from the node's position)
node->play_sound(explosion_sound_id);

// Ambient sound (same volume everywhere)
node->play_sound(music_id, smlt::AUDIO_REPEAT_FOREVER, smlt::DISTANCE_MODEL_AMBIENT);

// One-shot with repeat options
node->play_sound(sound_id, smlt::AUDIO_REPEAT_NONE);  // Plays once
node->play_sound(sound_id, smlt::AUDIO_REPEAT_FOREVER);  // Loops
```

### Playing Sounds Across Scene Transitions

`StageNode`-based audio is destroyed when the scene changes. For continuous audio (like background music), play through the `SoundDriver` directly:

```cpp
auto playing = application->sound_driver->play_sound(music_id);
// Music continues playing even when scenes change
application->sound_driver->stop_sound(playing);  // Stop when done
```

### Volume Control

```cpp
// Per-source volume (0.0 to 1.0)
node->set_gain(0.5f);

// Global volume through SoundDriver
application->sound_driver->set_master_volume(0.8f);
```

### Platform-Specific Audio Notes

- **Dreamcast:** File reads from CD can interrupt CD audio playback. Use `vfs->enable_read_blocking()` during music playback if needed.
- **PSP:** Audio is currently not fully implemented. WAV files may work but OGG is not supported.
- **Desktop:** Both OGG and WAV work reliably.

---

## 6. Fonts

### Supported Formats

| Format | Extension | Description |
|--------|-----------|-------------|
| **TrueType** | `.ttf` | Scalable vector fonts. Rendered at load time to the requested size. |
| **Bitmap Font** | `.fnt` | Pre-rendered bitmap fonts. Often generated by tools like BMFont. |

### Loading and Sizing Fonts

Fonts are loaded with `FontFlags` that control size, weight, style, and character set:

```cpp
smlt::FontFlags flags;
flags.size = 24;                          // Font size in points
flags.weight = smlt::FONT_WEIGHT_BOLD;    // Normal, Bold, Light
flags.style = smlt::FONT_STYLE_ITALIC;    // Normal, Italic
flags.charset = smlt::CHARACTER_SET_LATIN; // Character set to include

auto font = assets->load_font("fonts/Orbitron-BoldItalic.ttf", flags);
```

### Loading from System Fonts

You can search for fonts by family name:

```cpp
smlt::FontFlags flags;
flags.size = 18;
flags.weight = smlt::FONT_WEIGHT_NORMAL;

// Searches for: Kanit-Regular.ttf, Kanit-Regular-18.fnt, etc.
auto font = assets->create_font_from_family("Kanit", flags);
```

This searches multiple paths including `$path/Kanit-Regular.ttf`, `$path/fonts/Kanit/Kanit-Regular.ttf`, and bitmap variants.

### Loading from Memory

For embedded fonts (useful on Dreamcast/PSP):

```cpp
#include "fonts/orbitron_ttf.h"  // Generated by xxd -i

smlt::FontFlags flags;
flags.size = 16;
auto font = assets->create_font_from_memory(Orbitron_ttf, Orbitron_ttf_len, flags);
```

### Default Font

Simulant requires a default font. The built-in default is **Orbitron**. You can change it:

```cpp
assets->set_default_font_filename("fonts/MyCustomFont.ttf");
```

### Character Sets

The character set controls which glyphs are generated, affecting memory usage:

| Character Set | Description |
|--------------|-------------|
| `CHARACTER_SET_LATIN` | Basic Latin alphabet (ASCII + common punctuation) |
| Custom sets | Define based on your game's localization needs |

For memory-constrained platforms, only include the character sets you need.

---

## 7. Particle Scripts

### The .kglp Format

Simulant uses a custom JSON-based format for particle systems, typically saved with the `.kglp` extension (files are also loadable as `.script`).

### File Structure

```json
{
    "name": "Fire Effect",
    "quota": 200,
    "particle_width": 0.5,
    "particle_height": 0.5,
    "cull_each": false,
    "material": "TEXTURED_PARTICLE",
    "s_diffuse_map": "textures/particle_flame.png",
    "emitters": [
        {
            "type": "point",
            "direction": "0 1 0",
            "velocity": 2.0,
            "ttl_min": 0.5,
            "ttl_max": 1.5,
            "angle": 15.0,
            "colour": "1 0.5 0 1",
            "emission_rate": 50,
            "duration": 0,
            "repeat_delay": 0
        }
    ],
    "manipulators": [
        {
            "type": "size",
            "rate": -0.5
        },
        {
            "type": "direction",
            "force": "0 0.5 0"
        }
    ]
}
```

### System Properties

| Property | Type | Description |
|----------|------|-------------|
| `name` | string | Human-readable description |
| `quota` | integer | Maximum number of simultaneous particles across all emitters |
| `particle_width` | float | Width of particle sprites in world units |
| `particle_height` | float | Height of particle sprites in world units |
| `cull_each` | boolean | Whether each particle is individually culled (not yet implemented) |
| `material` | string | Path to a `.smat` file or built-in material name (e.g., `"TEXTURED_PARTICLE"`) |
| `emitters` | array | List of particle emitters |
| `manipulators` | array | List of rules that affect particles each frame |

### Emitter Types

| Type | Description |
|------|-------------|
| `point` | All particles originate from a single point |
| `box` | Particles originate from within a box volume |

### Emitter Properties

| Property | Type | Description |
|----------|------|-------------|
| `type` | string | `"point"` or `"box"` |
| `direction` | string | Space-separated `x y z` float values (relative direction) |
| `velocity` | float | Emission speed of particles |
| `ttl_min` | float | Minimum particle lifetime (seconds) |
| `ttl_max` | float | Maximum particle lifetime (seconds) |
| `angle` | float | Spread angle in degrees from direction vector |
| `colour` | string | Space-separated `r g b a` values (0.0-1.0) |
| `emission_rate` | integer | Particles emitted per second |
| `duration` | float | Emitter lifetime (0 = forever) |
| `repeat_delay` | float | Delay before emitter restarts (for burst effects) |

### Manipulator Types

| Type | Description | Key Properties |
|------|-------------|----------------|
| `size` | Changes particle size over time | `rate` (-1.0 to 1.0), or `curve` (linear/bell) |
| `colour_fader` | Fades particle color over time | `colours` (list of "R G B A" strings), `interpolate` |
| `direction` | Applies force to particles | `force` ("X Y Z" vector) |
| `direction_noise_random` | Adds random noise to direction | `force`, `noise_amount` |

### Loading and Using Particle Scripts

```cpp
// Load from file
auto fire = assets->load_particle_script("particles/fire.kglp");

// Load a built-in
auto builtin_fire = assets->load_particle_script(
    smlt::ParticleScript::BuiltIns::FIRE  // "particles/fire.kglp"
);

// Create a particle system node
auto particles = stage->create_child<smlt::ParticleSystem>();
particles->set_script(fire);
particles->start();
```

### Testing and Iteration

Particle effects often require rapid iteration:

1. **Edit the `.kglp` file** in your text editor
2. **Restart the scene** to reload the particle script (assets are reloaded)
3. **Adjust values incrementally** -- small changes in `velocity`, `angle`, and `ttl` have large visual impact
4. **Watch the quota** -- if particles are disappearing, increase `quota`

---

## 8. Asset Organization

### Recommended Directory Structure

```
mygame/
├── assets/
│   ├── fonts/                    # TTF and FNT files
│   │   ├── Orbitron.ttf
│   │   └── Kanit-Regular.ttf
│   ├── materials/                # .smat and .material files
│   │   ├── hero.smat
│   │   └── ground.smat
│   ├── meshes/                   # 3D model files
│   │   ├── characters/
│   │   │   ├── hero.glb
│   │   │   └── enemy.glb
│   │   ├── environments/
│   │   │   ├── level_01.glb
│   │   │   └── level_02.glb
│   │   └── props/
│   │       ├── crate.obj
│   │       └── barrel.obj
│   ├── particle_scripts/         # .kglp particle definitions
│   │   ├── fire.kglp
│   │   └── smoke.kglp
│   ├── sounds/
│   │   ├── music/                # OGG background music
│   │   │   ├── forest.ogg
│   │   │   └── cave.ogg
│   │   ├── sfx/                  # WAV sound effects
│   │   │   ├── explosion.wav
│   │   │   └── jump.wav
│   │   └── ui/
│   │       └── click.wav
│   ├── shaders/                  # GLSL shader files
│   │   ├── custom.vert
│   │   └── custom.frag
│   └── textures/
│       ├── characters/
│       │   ├── hero_diffuse.png
│       │   └── hero_normal.png
│       ├── environments/
│       │   └── ground_diffuse.png
│       └── ui/
│           ├── button.png
│           └── health_bar.png
```

### Naming Conventions

| Convention | Example | Notes |
|------------|---------|-------|
| **lowercase with underscores** | `hero_diffuse.png` | Consistent, no case-sensitivity issues across platforms |
| **Include type in name** | `hero_diffuse.png`, `hero_normal.png` | Distinguishes texture types at a glance |
| **Group by category** | `textures/characters/`, `meshes/environments/` | Keeps related assets together |
| **Avoid spaces and special characters** | `my_asset.png` not `my asset!.png` | Prevents path resolution issues |

### Asset Search Paths

Simulant automatically adds these search paths:

| Path | Platform |
|------|----------|
| Working directory | All |
| Executable directory | All (except Android) |
| `assets/` subdirectory | All |
| `simulant/` subdirectory | All |
| `/usr/local/share`, `/usr/share` | Linux |
| `/cd`, `/pc` | Dreamcast |
| `.`, `umd0:`, `ms0:`, `disc0:` | PSP |

Add custom search paths in your `AppConfig`:

```cpp
struct MyConfig : public smlt::AppConfig {
    MyConfig() {
        search_paths.push_back("assets");
        search_paths.push_back("/path/to/mod/data");
    }
};
```

### VFS Path Placeholders

The VFS expands `${PLATFORM}` and `${RENDERER}` in paths:

```cpp
// Tries: models/psp/hero.obj, then models/hero.obj
auto mesh = assets->load_mesh("models/${PLATFORM}/hero.obj");
```

This allows platform-specific asset overrides:

```
assets/
├── models/
│   ├── hero.obj              # Default mesh
│   └── psp/
│       └── hero.obj          # PSP-optimized version (lower poly)
```

---

## 9. Platform-Specific Considerations

### Platform Specifications

| Platform | CPU | RAM | Renderer | Notes |
|----------|-----|-----|----------|-------|
| **Linux/Windows** | Any modern | 1GB+ | GL2X (OpenGL 2.x) | Full feature support |
| **Dreamcast** | 200MHz | 16MB | GL1X (OpenGL ES 1.x) | 2 max lights per object, 1024 max texture size |
| **PSP** | 333MHz | 32MB | Custom PSP renderer | 4 max lights per object, 512 max texture size, no OGG audio |

### Desktop (Linux/Windows)

- **Full feature support:** All asset types, custom shaders, large textures
- **Runtime asset loading:** Assets loaded from disk, no embedding needed
- **Fast iteration:** Edit assets and restart without rebuilding
- **No strict size limits:** Use high-resolution textures, complex models
- **OGG and WAV audio:** Both formats fully supported
- **Custom GLSL shaders:** Full vertex and fragment shader support

```cpp
// Desktop-specific features
if (get_platform()->name() == "linux" || get_platform()->name() == "windows") {
    create_child<smlt::Skybox>("skyboxes/default");  // Large skybox textures
}
```

### Dreamcast

The Sega Dreamcast is a **highly constrained** target. Careful asset preparation is essential.

#### Constraints

| Constraint | Limit |
|------------|-------|
| **RAM** | 16MB total (shared with OS and game) |
| **Texture size** | 1024 pixels maximum |
| **Lights per object** | 2 maximum |
| **Renderer** | GL1X only (fixed-function, no custom shaders) |
| **Audio** | OGG and WAV supported |
| **Filesystem** | CD-ROM (slow seek times) |

#### Asset Embedding

Dreamcast has no filesystem in the traditional sense. Assets are **embedded into the executable** during the build process:

```bash
simulant build dreamcast
```

The build system automatically converts all assets in your `asset_paths` into C arrays and links them into the binary.

#### Optimization Tips

- **Reduce polygon counts** -- target 500-2000 triangles per visible mesh
- **Use smaller textures** -- 256x256 or 512x512 maximum
- **Avoid skyboxes** -- cube map textures may exceed memory limits
- **Use DTEX/KMG textures** -- native Dreamcast formats for better performance
- **Stream music carefully** -- CD file reads can interrupt CD audio
- **Keep total asset size reasonable** -- everything fits in the final `.bin` image

```cpp
#ifdef SIMULANT_PLATFORM_DREAMCAST
    // Skip large skyboxes
    // Use lower-poly meshes
    auto mesh = assets->load_mesh("meshes/hero_dc.obj");  // Dreamcast-specific version
#else
    create_child<smlt::Skybox>("skyboxes/TropicalSunnyDay");
    auto mesh = assets->load_mesh("meshes/hero.glb");
#endif
```

### PSP

The PSP has similar constraints but with some differences.

#### Constraints

| Constraint | Limit |
|------------|-------|
| **RAM** | 32MB total |
| **Texture size** | 512 pixels maximum, **must be power-of-two** |
| **Lights per object** | 4 maximum |
| **Audio** | WAV only (OGG not supported) |
| **Filesystem** | Memory Stick / UMD |

#### Asset Embedding

Like Dreamcast, PSP assets can be embedded:

```bash
simulant build psp
```

#### Optimization Tips

- **Power-of-two textures** are mandatory (256x256, 512x512, etc.)
- **Use WAV for all audio** -- OGG is not available
- **Reduce polygon counts** -- similar to Dreamcast
- **Avoid large textures** -- 512x512 maximum, prefer 256x256
- **Use platform-specific assets** via `${PLATFORM}` path placeholders

```cpp
#ifdef SIMULANT_PLATFORM_PSP
    // PSP requires WAV audio
    auto music = assets->load_sound("music/level.wav");
    // PSP requires power-of-two textures
    auto tex = assets->load_texture("textures/ground_256.png");
#endif
```

---

## 10. Build Process and Asset Embedding

### Desktop Build (Runtime Loading)

On desktop platforms, assets are loaded from disk at runtime:

```bash
simulant build
simulant run
```

Your project structure should have the `assets/` directory alongside the executable, or listed in `asset_paths` in `simulant.json`.

### Asset Embedding for Constrained Platforms

For Dreamcast and PSP, Simulant embeds assets into the executable during the build process:

```bash
# Dreamcast build (requires Docker with kazade/dreamcast-sdk)
simulant build dreamcast

# PSP build
simulant build psp
```

The build system uses the `simulant.json` configuration to determine which assets to embed:

```json
{
    "name": "mygame",
    "asset_paths": ["assets"],
    "core_assets": true,
    "target_platforms": ["linux", "dreamcast", "psp"]
}
```

### Manual Embedding with bin2c / xxd

You can also manually embed individual files using standard tools:

```bash
# Convert any file to a C header using xxd
xxd -i fonts/Orbitron.ttf > fonts/orbitron_ttf.h

# Or use bin2c
bin2c fonts/Orbitron.ttf > fonts/orbitron_ttf.h
```

This generates a header file:

```c
// orbitron_ttf.h
unsigned char Orbitron_ttf[] = {
    0x00, 0x01, 0x00, 0x00, 0x00, 0x0e, ...
};
unsigned int Orbitron_ttf_len = 45678;
```

Load the embedded data in your code:

```cpp
#include "fonts/orbitron_ttf.h"

smlt::FontFlags flags;
flags.size = 24;
auto font = assets->create_font_from_memory(Orbitron_ttf, Orbitron_ttf_len, flags);
```

This approach is useful for:
- Individual fonts
- Small configuration files
- Custom binary data

### Asset Optimization

Before embedding assets for constrained platforms:

1. **Reduce polygon counts** -- use decimation tools in Blender or MeshLab
2. **Resize textures** -- use ImageMagick or similar:
   ```bash
   magick convert texture.png -resize 256x256 texture_small.png
   ```
3. **Compress textures** -- convert PNG to JPG for photos:
   ```bash
   magick convert photo.png -quality 85 photo.jpg
   ```
4. **Combine texture sheets** -- use sprite/texture atlases to reduce draw calls
5. **Remove unused assets** -- every embedded asset increases binary size
6. **Use platform-specific variants** -- lower-detail meshes and smaller textures for Dreamcast/PSP

### Checking Asset Size

Monitor the size of your embedded assets:

```cpp
// Check available memory
S_INFO("Total RAM: {} bytes", get_platform()->total_ram_in_bytes());
S_INFO("Available RAM: {} bytes", get_platform()->available_ram_in_bytes());

// Monitor asset counts
S_INFO("Meshes: {}", assets->mesh_count());
S_INFO("Textures: {}", assets->texture_count());
S_INFO("Materials: {}", assets->material_count());
```

---

## 11. Best Practices for Asset Management

### 1. Store AssetIDs, Not Shared Pointers

```cpp
// BAD: Holding shared_ptr prevents garbage collection
class Player {
    smlt::MeshPtr mesh_;  // Ref count never drops to 1
};

// GOOD: Store the ID, borrow the pointer when needed
class Player {
    smlt::MeshID mesh_id_;

    void update(smlt::AssetManager* assets) {
        auto mesh = assets->mesh(mesh_id_);  // Borrow temporarily
        // Use mesh...
    }  // Released at end of scope
};
```

### 2. Use Scope Blocks for Asset Access

Keep asset pointers alive for the shortest time possible:

```cpp
// GOOD: Scope block releases pointer immediately
{
    auto mat = assets->material(mat_id_);
    mat->set_diffuse_color(color);
}

// GOOD: Single-line for one-shot operations
assets->mesh(mesh_id_)->recalculate_normals();
```

### 3. Choose the Right AssetManager

| Asset Type | Use | Reason |
|------------|-----|--------|
| UI fonts and textures | `shared_assets` | Needed by all scenes |
| Common sound effects | `shared_assets` | Used everywhere |
| Level meshes | `scene->assets` | Only needed for this scene |
| Level-specific music | `scene->assets` | Changes per scene |
| Reusable projectiles | `shared_assets` + `GARBAGE_COLLECT_NEVER` | Spawns/despawns repeatedly |

### 4. Name Your Assets

Descriptive names help with debugging:

```cpp
auto mesh = assets->load_mesh("hero.glb");
mesh->set_name("HeroMesh");

auto tex = assets->load_texture("grass.png");
tex->set_name("GrassTile_Diffuse");

// Find by name later
auto found = assets->find_mesh("HeroMesh");
```

### 5. Pre-Load in on_load(), Use in on_activate()

```cpp
class GameScene : public smlt::Scene {
protected:
    void on_load() override {
        // Heavy loading -- happens during loading screen
        level_mesh_ = assets->load_mesh("levels/forest.glb");
        hero_tex_ = assets->load_texture("textures/hero_diffuse.png");
        bgm_ = assets->load_sound("music/forest.ogg");
    }

    void on_activate() override {
        // Fast -- assets are already loaded
        auto hero = create_child<smlt::Actor>(hero_tex_);
        auto level = create_child<smlt::Actor>(level_mesh_);
    }

    void on_unload() override {
        // Automatic cleanup -- no manual work needed
    }

private:
    smlt::MeshID level_mesh_;
    smlt::TextureID hero_tex_;
    smlt::SoundID bgm_;
};
```

### 6. Disable GC for Frequently Reused Assets

```cpp
// Torpedo mesh is loaded once, reused many times
auto torpedo = assets->load_mesh(
    "torpedo.glb",
    smlt::VertexSpecification::DEFAULT,
    smlt::MeshLoadOptions(),
    GARBAGE_COLLECT_NEVER
);

// When permanently done:
torpedo->set_garbage_collection_method(GARBAGE_COLLECT_PERIODIC);
assets->destroy_mesh(torpedo->id());
```

### 7. Use Platform-Specific Assets

```cpp
// Let VFS handle platform selection
// Tries models/psp/hero.obj first, then falls back to models/hero.obj
auto hero = assets->load_mesh("models/${PLATFORM}/hero.obj");
```

### 8. Keep a Clean Asset Directory

- Remove unused assets regularly
- Use version control for source files (.blend, .psd) but not for generated files
- Document texture maps with consistent naming (`_diffuse`, `_normal`, `_specular`)

---

## 12. Common Issues and Solutions

### Model Does Not Appear

| Possible Cause | Solution |
|----------------|----------|
| Asset not stored | Ensure you store the return value: `auto mesh = assets->load_mesh(...)` |
| Asset garbage collected | Attach to an Actor, or use `GARBAGE_COLLECT_NEVER` |
| Wrong path | Check the path is relative to a search path. Use `vfs->locate_file("path")` to debug |
| Wrong scale | Check model scale. Use `actor->transform->set_scale()` to adjust |
| Backface culling | Check `cull_mode` in MeshLoadOptions. Try `CULL_MODE_NONE` |
| No material | Ensure the model has a material assigned. Use `Material::BuiltIns::DEFAULT` |

### Model Renders Incorrectly

| Possible Cause | Solution |
|----------------|----------|
| Wrong coordinate system | Re-export with **Y-up** enabled |
| Missing normals | Enable "Write Normals" in exporter |
| Missing UVs | Enable "Write UVs" in exporter |
| IK constraints | Bake IK to keyframes before exporting |
| Textures missing | Check texture paths in the model file. Use `override_texture_extension` if needed |

### Texture Does Not Appear

| Possible Cause | Solution |
|----------------|----------|
| Non-power-of-two on PSP | Resize to power-of-two (256, 512, etc.) |
| Wrong path | Verify path through VFS. Check search paths |
| Material not set | Ensure material has `s_diffuse_map` set |
| UV mapping issue | Check UV coordinates in the 3D model |
| Alpha blending | Check `s_blend_func` property in material |

### Texture Looks Wrong on PSP

| Possible Cause | Solution |
|----------------|----------|
| Non-power-of-two dimensions | PSP requires power-of-two textures |
| Too large (over 512) | Resize to 512x512 or smaller |

### Sound Does Not Play

| Possible Cause | Solution |
|----------------|----------|
| OGG on PSP | Use WAV format instead |
| Not stored | Store the SoundID returned by `load_sound()` |
| Volume at zero | Check `set_gain()` and master volume |
| Streamed audio disposed | Ensure the sound file stays accessible during playback |

### Font Does Not Render

| Possible Cause | Solution |
|----------------|----------|
| Missing glyphs | Check `charset` in FontFlags includes the characters you need |
| Wrong size | Check `FontFlags::size` is set appropriately |
| Font file not found | Verify path. Use `create_font_from_memory()` for embedded fonts |

### Particle System Not Visible

| Possible Cause | Solution |
|----------------|----------|
| Quota too low | Increase `quota` in the `.kglp` file |
| Wrong material | Ensure material is valid. Try `"TEXTURED_PARTICLE"` as a test |
| Emitter duration = 0 | Duration of 0 means forever -- that should be fine, but check other values |
| Particles off-screen | Check emitter `direction` and `velocity` |
| Material blending issue | Check the material's `s_blend_func` setting |

### Performance Issues

| Symptom | Solution |
|---------|----------|
| Low frame rate on Dreamcast | Reduce polygon counts, use smaller textures, reduce light count |
| Low frame rate on PSP | Ensure power-of-two textures, reduce resolution, check texture sizes <= 512 |
| Stuttering during loading | Pre-load assets in `on_load()`, not `on_activate()` |
| Memory exhaustion | Check texture sizes, reduce quota on particle systems, stream music instead of loading into memory |
| Long load times | Use compressed texture formats (DDS), reduce total asset count |

### Build/Embedding Issues

| Problem | Solution |
|---------|----------|
| Assets missing on Dreamcast | Check `asset_paths` in `simulant.json` includes your asset directories |
| Binary too large | Reduce texture sizes, remove unused assets, use platform-specific lower-detail variants |
| Assets load on desktop but not embedded | Verify paths are relative, not absolute |

---

## Quick Reference

### Asset Loading Summary

```cpp
// Meshes
MeshPtr m = assets->load_mesh("model.glb");
MeshPtr m = assets->load_mesh("model.obj", spec, options, GARBAGE_COLLECT_NEVER);

// Textures
TexturePtr t = assets->load_texture("image.png");
TexturePtr t = assets->load_texture("image.png", flags);

// Materials
MaterialPtr mt = assets->load_material("mat.smat");
MaterialPtr mt = assets->create_material();
MaterialPtr mt = assets->create_material_from_texture(tex);

// Sounds
SoundPtr s = assets->load_sound("audio.ogg");
SoundPtr s = assets->load_sound("audio.wav", sound_flags);

// Fonts
FontPtr f = assets->load_font("font.ttf", font_flags);
FontPtr f = assets->create_font_from_family("Kanit", font_flags);
FontPtr f = assets->create_font_from_memory(data, size, font_flags);

// Particle Scripts
ParticleScriptPtr ps = assets->load_particle_script("fire.kglp");
ParticleScriptPtr ps = assets->load_particle_script(smlt::ParticleScript::BuiltIns::FIRE);
```

### File Format Quick Reference

| Asset Type | Recommended Format | Alternative Formats |
|------------|-------------------|---------------------|
| 3D Models | glTF 2.0 (.glb) | OBJ, MS3D, MD2 |
| Textures | PNG | JPG, DDS, DTEX, KMG |
| Materials | .smat (JSON) | .material |
| Music | OGG | WAV (for PSP) |
| Sound Effects | WAV | OGG |
| Fonts | TTF | FNT |
| Particles | .kglp (JSON) | .script |
| Shaders | .vert / .frag (GLSL) | Built-in only for GL1X |

### Platform Limits Summary

| Limit | Desktop | Dreamcast | PSP |
|-------|---------|-----------|-----|
| Max texture size | Driver-dependent | 1024 | 512 |
| Power-of-two required | No | Recommended | **Yes** |
| Max lights per object | 8 | 2 | 4 |
| OGG audio | Yes | Yes | **No** |
| Custom shaders | Yes (GL2X) | No (GL1X fixed) | Limited |
| Asset loading | Runtime from disk | Embedded in binary | Embedded in binary |
