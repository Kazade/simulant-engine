# sprite_gen: Generating Sprite Sheets from 3D Models

`sprite_gen` is a command-line tool included with Simulant that renders an animation from a 3D model into a 2D sprite sheet, optionally alongside a JSON atlas describing each frame. This is the classic "pre-rendered 2D sprite" workflow -- animate a rigged 3D character once, then render it out to a sprite sheet for use in a 2D or 2.5D game.

**Related documentation:** [Spritesheet Asset & Atlas Format](../assets/spritesheets.md), [Sprites](../rendering/sprites.md), [Sprite Animation](../animation/sprite-animation.md), [Asset Pipeline Guide](asset-pipeline.md).

---

## 1. What It Does

`sprite_gen`:

1. Loads a `.glb`/`.gltf` model (the same loader Simulant uses at runtime -- see [Asset Pipeline: 3D Model Preparation](asset-pipeline.md#2-3d-model-preparation)).
2. Plays one of its animations, sampling it at a fixed rate.
3. Renders each sampled frame from a fixed camera into an off-screen buffer.
4. Packs all the frames as tightly as possible into a single, power-of-two texture (up to 1024x1024), leaving a small transparent border around each frame to avoid bilinear filtering bleeding between neighbours.
5. Writes the result as an uncompressed `.tga` image, and (optionally) a TexturePacker-style JSON atlas describing every frame's position and a single animation entry covering the whole clip.

The output atlas is loadable directly with `AssetManager::load_spritesheet()` -- see [Spritesheet Asset & Atlas Format](../assets/spritesheets.md) for the format and the runtime API.

## 2. Building

`sprite_gen` is built as part of the normal Simulant CMake build (source: `tools/sprite_gen/`) and installed as a regular executable:

```bash
cmake --build build --target sprite_gen
```

The resulting binary is `sprite_gen` (or `sprite_gen.exe` on Windows).

## 3. Usage

```
sprite_gen -i <input.glb|.gltf> [options]
```

| Option | Description |
|--------|-------------|
| `-i, --input <path>` | Input `.glb`/`.gltf` file. **Required.** |
| `-o, --output <path>` | Output `.tga` file. Default: `<input>.tga`. |
| `-a, --animation <name>` | Animation to render. Default: the first animation found in the model. |
| `-r, --rotation <x,y,z>` | Euler rotation in degrees applied to the model before rendering. Default: `0,0,0`. |
| `-s, --scale <factor>` | Uniform scale applied to the model. Default: `1.0`. |
| `--light-color <r,g,b>` | Light color, `0-1` floats. Default: `1,1,1`. |
| `--light-direction <x,y,z>` | Direction for a directional light. |
| `--light-position <x,y,z>` | Position for a point light. Overrides `--light-direction`. |
| `--ambient <r,g,b>` | Ambient light color, `0-1` floats. Default: `1,1,1` (full brightness). |
| `--fps <n>` | Animation sampling rate -- how many frames per second of animation to capture. Default: `15`. |
| `--atlas <path>` | Also write a JSON atlas describing the sheet. If omitted, only the `.tga` is written. |
| `-h, --help` | Show usage. |

If neither `--light-direction` nor `--light-position` is given, the model is rendered with ambient light only.

### Example

```bash
sprite_gen -i models/hero.glb \
    -a walk \
    -o assets/sprites/hero_walk.tga \
    --atlas assets/sprites/hero_walk.json \
    --fps 12 \
    --light-direction 0.3,-1,0.2 \
    --ambient 0.3,0.3,0.35
```

This renders the `walk` animation from `hero.glb` at 12 samples per second, writing `hero_walk.tga` and `hero_walk.json` into `assets/sprites/`.

## 4. Loading the Result in Simulant

```cpp
auto sheet = assets->load_spritesheet("sprites/hero_walk.json");

auto sprite = create_child<Sprite>(Params().set("spritesheet", sheet));
sprite->set_render_dimensions(1.0f, 1.0f);
sprite->animations->play_animation("walk"); // Named after the -a/--animation value
```

`sprite_gen` writes exactly one `frameTags` entry per run, named after the animation it rendered (the `-a`/`--animation` value, or the model's first animation if omitted), covering every frame it captured. If you want to combine multiple animations (e.g. `walk` and `jump`) into a single sprite sheet, run `sprite_gen` once per animation with the same `--output`/`--atlas` base name convention you prefer, then either load each atlas as a separate `Spritesheet`, or hand-merge the JSON files and re-pack the images yourself.

## 5. Notes and Limitations

- **Frame naming:** Frames in the generated atlas are named `<animation>_<index>` (e.g. `walk_0`, `walk_1`, ...), where `<index>` is 0-based.
- **Sheet size:** The packed sheet is capped at 1024x1024 and rounded up to a power of two, which keeps it compatible with constrained platforms (see [Asset Pipeline: Platform-Specific Considerations](asset-pipeline.md#9-platform-specific-considerations)). If your animation has too many frames or is rendered at too high a resolution to fit, reduce `--fps` or split the animation across multiple runs.
- **Frame padding:** A small transparent border (2px) is reserved around each frame's content to prevent bilinear filtering from sampling neighbouring frames.
- **Output format:** Only uncompressed `.tga` output is supported. Convert to another format afterwards if needed (e.g. with ImageMagick).
- **Camera framing:** The camera is automatically framed to fit the model's animated bounding box across every sampled frame, so the model doesn't clip in or out of view during the animation.

---

## See Also

- [Spritesheet Asset & Atlas Format](../assets/spritesheets.md) -- The JSON atlas format and runtime loading API
- [Sprites](../rendering/sprites.md) -- General 2D sprite rendering guide
- [Sprite Animation](../animation/sprite-animation.md) -- Playing and controlling animations
- [Asset Pipeline Guide](asset-pipeline.md) -- Preparing 3D models and textures
