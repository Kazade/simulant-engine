# Material Files (.smat)

Material files define how meshes are rendered. They specify colors, textures, blending modes, GPU programs (shaders), and multi-pass rendering configurations. Material files in Simulant use the `.smat` extension and are written in JSON.

**Related documentation:**
- [Asset Management](asset-managers.md) -- Loading and managing materials programmatically
- [Virtual File System](vfs.md) -- How the VFS resolves file references within material scripts
- [Mesh Formats](mesh-formats.md) -- Supported mesh file formats

---

## 1. What Is a Material?

A **Material** describes the visual appearance of a mesh. It determines:

- **Base color**: The primary color of the surface.
- **Textures**: Base color maps, normal maps, light maps, and metallic-roughness maps.
- **Metallic and roughness**: PBR-style material properties for realistic lighting.
- **Specular**: Specular intensity and color for non-PBR workflows.
- **Blending**: Whether and how the material blends with what is already rendered.
- **Depth testing and writing**: How the material interacts with the depth buffer.
- **Culling**: Which faces are rendered (front, back, none, or both).
- **GPU programs**: Custom vertex and fragment shaders for per-pass rendering.
- **Custom properties**: User-defined values passed to GPU programs.

Materials can be loaded from `.smat` files or created programmatically. This document covers the file format.

---

## 2. Loading a Material File

Materials are loaded through the `AssetManager`:

```cpp
// Load from a .smat file
MaterialPtr mat = assets->load_material("materials/hero.smat");

// Materials can also be loaded from built-in paths using placeholders
MaterialPtr default_mat = assets->load_material("materials/${RENDERER}/default.smat");
```

The loader reads the JSON file, registers any custom properties, configures the material's core properties, and sets up rendering passes (including GPU programs if specified).

### Built-In Materials

The engine ships with three built-in materials that you can reference by name:

| Name | Path | Description |
|------|------|-------------|
| `DEFAULT` | `materials/${RENDERER}/default.smat` | Default general-purpose material |
| `TEXTURE_ONLY` | `materials/${RENDERER}/texture_only.smat` | Renders a texture without lighting |
| `DIFFUSE_ONLY` | `materials/${RENDERER}/diffuse_only.smat` | Diffuse-only lighting model |

You can reference these by name in code or in particle scripts:

```cpp
auto mat = assets->load_material(Material::BuiltIns::DEFAULT);
```

---

## 3. The .smat File Format

A `.smat` file is a JSON object with the following top-level keys:

| Key | Type | Required | Description |
|-----|------|----------|-------------|
| `passes` | array | Yes | An array of rendering passes. Each pass can have its own GPU program and property overrides. |
| `custom_properties` | array | No | Defines custom material properties with types and default values. |
| `property_values` | object | No | Sets values for core or custom properties at the material level. |

### Minimal Example

```json
{
    "passes": [
        {}
    ]
}
```

This defines a material with a single pass that uses the renderer's default GPU program and default property values (white base color, no textures, standard depth/cull settings).

---

## 4. Custom Properties

Custom properties let you define named values that are accessible from GPU programs. They are declared in the `custom_properties` array and can have their values set in `property_values`.

### Declaring Custom Properties

Each entry in `custom_properties` is an object with `name`, `type`, and `default`:

```json
{
    "custom_properties": [
        {
            "name": "emissive_strength",
            "type": "float",
            "default": 1.0
        },
        {
            "name": "emissive_map",
            "type": "texture",
            "default": "textures/glow.png"
        },
        {
            "name": "use_wireframe",
            "type": "bool",
            "default": false
        },
        {
            "name": "layer_count",
            "type": "int",
            "default": 3
        }
    ],
    "passes": [{}]
}
```

### Supported Property Types

| Type | Description | JSON Value Format |
|------|-------------|-------------------|
| `bool` | Boolean value | `true` or `false` |
| `float` | Floating-point number | Number (e.g., `0.5`) |
| `int` | Integer number | Number (e.g., `42`) |
| `texture` | Texture reference | String path to an image file (e.g., `"textures/noise.png"`) |
| `vec2` | Two-component vector | Not directly usable in `custom_properties` defaults via JSON (use in `property_values` as space-separated string) |
| `vec3` | Three-component vector | Same as above |
| `vec4` | Four-component vector | Same as above |

---

## 5. Property Values

The `property_values` object at the root level sets values for both core material properties and custom properties.

### Core Material Properties

The engine defines a set of core properties for every material. These can be overridden via `property_values`:

| Property Name | Type | Default | Description |
|---------------|------|---------|-------------|
| `s_material_base_color` | vec4 (color) | `1 1 1 1` (white) | The base color of the material |
| `s_material_specular_color` | vec4 (color) | `0 0 0 1` (black) | Specular highlight color |
| `s_material_specular` | float | `0.0` | Specular intensity |
| `s_material_roughness` | float | `0.4` | Surface roughness (0 = mirror, 1 = matte) |
| `s_material_metallic` | float | `0.0` | Metallic factor (0 = dielectric, 1 = metal) |
| `s_base_color_map` | texture | none | Base color / albedo texture |
| `s_light_map` | texture | none | Light map texture |
| `s_normal_map` | texture | none | Normal map texture |
| `s_metallic_roughness_map` | texture | none | Combined metallic-roughness texture |
| `s_depth_write_enabled` | bool | `true` | Whether to write to the depth buffer |
| `s_depth_test_enabled` | bool | `true` | Whether depth testing is enabled |
| `s_depth_func` | string | `"LESS_EQUAL"` | Depth comparison function |
| `s_blend_func` | string | `"NONE"` | Blending mode |
| `s_alpha_threshold` | float | `1.0` | Alpha cutoff for alpha testing |
| `s_cull_mode` | string | `"NONE"` | Face culling mode |
| `s_shade_model` | string | `"SMOOTH"` | Shading model |
| `s_lighting_enabled` | bool | `true` | Whether lighting calculations are applied |
| `s_textures_enabled` | int | (mask) | Bitmask of enabled texture slots |
| `s_point_size` | float | `1.0` | Point sprite size |
| `s_polygon_mode` | string | `"FILL"` | Polygon rendering mode |
| `s_color_material` | string | `"NONE"` | Color material source |
| `s_fog_mode` | string | `"NONE"` | Fog mode |
| `s_fog_density` | float | `1.0` | Fog density |
| `s_fog_start` | float | `100.0` | Fog start distance |
| `s_fog_end` | float | `1000.0` | Fog end distance |
| `s_fog_color` | vec4 (color) | `1 1 1 1` | Fog color |

### Setting Property Values

Property values are specified in the `property_values` object. The JSON value format depends on the property type:

```json
{
    "property_values": {
        "s_material_base_color": "0.8 0.2 0.2 1.0",
        "s_material_roughness": 0.6,
        "s_material_metallic": 0.3,
        "s_base_color_map": "textures/brick_albedo.png",
        "s_normal_map": "textures/brick_normal.png",
        "s_blend_func": "ALPHA",
        "s_cull_mode": "BACK_FACE",
        "s_lighting_enabled": true
    },
    "passes": [{}]
}
```

### String-Based Enums

Several core properties accept named string values for enum-like behavior:

**`s_blend_func`** -- Blending modes:

| Value | Description |
|-------|-------------|
| `NONE` | No blending (opaque) |
| `MASK` | Alpha testing (discard fragments below threshold) |
| `ADD` | Additive blending |
| `MODULATE` | Multiply blending |
| `COLOR` | Color blending |
| `ALPHA` | Standard alpha blending |
| `ONE_ONE_MINUS_ALPHA` | Src * 1 + Dst * (1 - src_alpha) |

**`s_cull_mode`** -- Face culling:

| Value | Description |
|-------|-------------|
| `NONE` | No culling (all faces rendered) |
| `BACK_FACE` | Back faces culled (default for solid objects) |
| `FRONT_FACE` | Front faces culled |
| `FRONT_AND_BACK_FACE` | No faces rendered |

**`s_shade_model`** -- Shading model:

| Value | Description |
|-------|-------------|
| `SMOOTH` | Smooth/Phong shading |
| `FLAT` | Flat shading |

**`s_polygon_mode`** -- Polygon rendering mode:

| Value | Description |
|-------|-------------|
| `FILL` | Filled polygons |
| `LINE` | Wireframe lines |
| `POINT` | Point vertices |

**`s_depth_func`** -- Depth comparison function:

| Value | Description |
|-------|-------------|
| `NEVER` | Never pass depth test |
| `LESS` | Pass if fragment depth is less |
| `LESS_EQUAL` | Pass if fragment depth is less than or equal (default) |
| `EQUAL` | Pass if fragment depth is equal |
| `GEQUAL` | Pass if fragment depth is greater than or equal |
| `GREATER` | Pass if fragment depth is greater |
| `ALWAYS` | Always pass |

**`s_fog_mode`** -- Fog mode:

| Value | Description |
|-------|-------------|
| `NONE` | No fog |
| `LINEAR` | Linear fog based on distance |
| `EXP` | Exponential fog |
| `EXP2` | Exponential squared fog |

**`s_color_material`** -- Color material source:

| Value | Description |
|-------|-------------|
| `NONE` | Color comes from material properties |
| `AMBIENT` | Color from ambient component |
| `DIFFUSE` | Color from diffuse component |
| `AMBIENT_AND_DIFFUSE` | Color from ambient + diffuse |

### Vector Formats

`vec3` and `vec4` properties (like colors) are written as space-separated strings:

```json
{
    "property_values": {
        "s_material_base_color": "1.0 0.5 0.0 1.0",
        "s_fog_color": "0.6 0.7 0.8 1.0"
    }
}
```

### Texture References

Texture properties are set with string paths relative to the VFS search paths:

```json
{
    "property_values": {
        "s_base_color_map": "textures/brick_albedo.png",
        "s_normal_map": "textures/brick_normal.png",
        "s_light_map": "textures/baked_lighting.png",
        "s_metallic_roughness_map": "textures/metallic_roughness.png"
    }
}
```

The texture paths are resolved through the VFS at load time.

---

## 6. Rendering Passes

The `passes` array defines one or more rendering passes. Each pass is rendered sequentially, allowing effects like multi-layer rendering, post-processing, or custom shader pipelines.

### Pass Structure

Each pass is a JSON object that can contain:

| Key | Type | Required | Description |
|-----|------|----------|-------------|
| `iteration` | string | No | How the pass iterates. Default: `"once"`. |
| `vertex_shader` | string | No | Path to a vertex shader file. |
| `fragment_shader` | string | No | Path to a fragment (pixel) shader file. |
| `property_values` | object | No | Per-pass property overrides. |

### Iteration Types

| Value | Description |
|-------|-------------|
| `once` | The pass renders exactly once per frame (default). |
| `once_per_light` | The pass renders once for each light affecting the mesh. |

### Basic Single-Pass Material

```json
{
    "passes": [
        {
            "property_values": {
                "s_material_base_color": "0.2 0.6 0.2 1.0",
                "s_material_roughness": 0.8
            }
        }
    ]
}
```

### Multi-Pass Material with Custom Shaders

```json
{
    "custom_properties": [
        {
            "name": "time",
            "type": "float",
            "default": 0.0
        },
        {
            "name": "glow_map",
            "type": "texture",
            "default": "textures/glow.png"
        }
    ],
    "property_values": {
        "s_base_color_map": "textures/character_albedo.png",
        "s_normal_map": "textures/character_normal.png"
    },
    "passes": [
        {
            "vertex_shader": "shaders/character.vert.glsl",
            "fragment_shader": "shaders/character.frag.glsl",
            "iteration": "once",
            "property_values": {
                "time": 1.5
            }
        },
        {
            "vertex_shader": "shaders/glow.vert.glsl",
            "fragment_shader": "shaders/glow.frag.glsl",
            "iteration": "once",
            "property_values": {
                "s_blend_func": "ADD"
            }
        }
    ]
}
```

This material has two passes:
1. The first renders the character with a custom shader.
2. The second adds a glow effect using additive blending.

### Shader File Paths

Shader file paths (`vertex_shader` and `fragment_shader`) are resolved relative to the **directory containing the `.smat` file**. The VFS temporarily adds the material file's parent directory to its search paths during loading, so you can reference shaders in the same directory or in subdirectories:

```json
{
    "passes": [
        {
            "vertex_shader": "shaders/pbr.vert.glsl",
            "fragment_shader": "shaders/pbr.frag.glsl"
        }
    ]
}
```

If your material is at `materials/pbr/character.smat`, the engine will look for `materials/pbr/shaders/pbr.vert.glsl` and `materials/pbr/shaders/pbr.frag.glsl`.

---

## 7. Per-Pass Property Overrides

Each pass can override property values set at the material level. This is useful for multi-pass materials where different passes need different settings:

```json
{
    "property_values": {
        "s_material_base_color": "1.0 1.0 1.0 1.0",
        "s_base_color_map": "textures/base.png",
        "s_depth_write_enabled": true,
        "s_blend_func": "NONE"
    },
    "passes": [
        {
            "property_values": {
                "s_blend_func": "NONE",
                "s_depth_write_enabled": true
            }
        },
        {
            "property_values": {
                "s_blend_func": "ADD",
                "s_depth_write_enabled": false
            }
        }
    ]
}
```

In this example:
- The first pass writes to the depth buffer and is opaque.
- The second pass adds on top without writing to the depth buffer.

---

## 8. Complete Example: PBR-Like Material

Here is a more complete material file that demonstrates most of the concepts:

```json
{
    "custom_properties": [
        {
            "name": "emissive_strength",
            "type": "float",
            "default": 0.0
        },
        {
            "name": "emissive_map",
            "type": "texture",
            "default": null
        }
    ],
    "property_values": {
        "s_material_base_color": "0.9 0.9 0.9 1.0",
        "s_material_roughness": 0.5,
        "s_material_metallic": 0.0,
        "s_material_specular": 0.5,
        "s_base_color_map": "textures/stone_albedo.png",
        "s_normal_map": "textures/stone_normal.png",
        "s_metallic_roughness_map": "textures/stone_mr.png",
        "s_cull_mode": "BACK_FACE",
        "s_blend_func": "NONE",
        "s_lighting_enabled": true,
        "s_depth_test_enabled": true,
        "s_depth_write_enabled": true
    },
    "passes": [
        {
            "vertex_shader": "shaders/pbr.vert.glsl",
            "fragment_shader": "shaders/pbr.frag.glsl",
            "iteration": "once"
        }
    ]
}
```

---

## 9. Creating Materials Programmatically

Not all materials need to be defined in files. You can create and configure materials entirely in code:

```cpp
// Create a blank material
MaterialPtr mat = assets->create_material();

// Set PBR properties
mat->set_base_color(smlt::Color(0.8f, 0.2f, 0.2f, 1.0f));
mat->set_metallic(0.3f);
mat->set_roughness(0.6f);
mat->set_specular(0.5f);
mat->set_specular_color(smlt::Color(1.0f, 0.9f, 0.8f, 1.0f));

// Set textures
auto albedo = assets->load_texture("textures/brick.png");
auto normal = assets->load_texture("textures/brick_normal.png");
mat->set_base_color_map(albedo);
mat->set_normal_map(normal);

// Set rendering state
mat->set_blend_func(smlt::BLEND_ALPHA);
mat->set_cull_mode(smlt::CULL_MODE_BACK_FACE);
mat->set_depth_test_enabled(true);
mat->set_lighting_enabled(true);
```

### Convenience Methods

```cpp
// Create a material from a single texture
MaterialPtr textured = assets->create_material_from_texture(my_texture);

// Clone an existing material
MaterialPtr variant = assets->clone_material(original->id());
variant->set_base_color(smlt::Color(0.2f, 0.8f, 0.2f, 1.0f));

// Clone the default material
MaterialPtr default_clone = assets->clone_default_material();
```

---

## 10. Material File Organization

### Recommended directory structure

```
assets/
  materials/
    ${RENDERER}/
      default.smat
      texture_only.smat
      diffuse_only.smat
    pbr/
      metal.smat
      plastic.smat
      glass.smat
    ui/
      flat_color.smat
      textured.smat
    shaders/
      pbr.vert.glsl
      pbr.frag.glsl
      flat.vert.glsl
      flat.frag.glsl
```

### Naming conventions

- Use descriptive names that indicate the material's purpose or visual style: `pbr_metal.smat`, `ui_flat_color.smat`, `water_transparent.smat`.
- Keep shader files alongside the materials that use them, or in a shared `shaders/` subdirectory.

---

## 11. Summary

| Concept | JSON Key | Notes |
|---------|----------|-------|
| Rendering passes | `passes` | Array of pass objects; each can have its own shaders and property overrides. |
| Custom properties | `custom_properties` | Named typed values with defaults; accessible from GPU programs. |
| Property values | `property_values` | Sets core and custom property values; can be overridden per-pass. |
| GPU programs | `vertex_shader`, `fragment_shader` | Paths to shader files, resolved relative to the `.smat` file. |
| Iteration mode | `iteration` | `"once"` or `"once_per_light"`. |
| Texture references | Property values of type `texture` | Resolved through the VFS. |
| Enum properties | String values | Blend functions, cull modes, depth functions, etc. |
