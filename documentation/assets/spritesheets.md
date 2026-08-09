# Spritesheet Asset & Atlas Format

`Spritesheet` is an asset type that describes a set of named, arbitrarily-placed frames on a single texture, plus any named animations (frame ranges) defined for it. It is loaded from a JSON atlas file and is the recommended way to drive a `Sprite` node when your frames aren't laid out in a uniform grid -- for example, sheets produced by the [`sprite_gen`](../guides/sprite-gen.md) tool or exported from tools like TexturePacker or Aseprite.

**Related documentation:** [Sprites](../rendering/sprites.md), [Sprite Animation](../animation/sprite-animation.md), [sprite_gen Tool](../guides/sprite-gen.md), [Asset Managers](asset-managers.md).

---

## Table of Contents

1. [Overview](#1-overview)
2. [The Atlas JSON Format](#2-the-atlas-json-format)
3. [Loading a Spritesheet](#3-loading-a-spritesheet)
4. [The Spritesheet API](#4-the-spritesheet-api)
5. [Using a Spritesheet with Sprite](#5-using-a-spritesheet-with-sprite)
6. [Spritesheet vs. the Grid-Based API](#6-spritesheet-vs-the-grid-based-api)

---

## 1. Overview

Headers:
- `simulant/assets/spritesheet.h` -- `Spritesheet`, `SpritesheetFrame`, `SpritesheetAnimation`
- `simulant/asset_manager.h` -- `AssetManager::load_spritesheet()`
- `simulant/nodes/sprite.h` -- `Sprite::set_spritesheet(SpritesheetPtr)`

Unlike `Sprite::set_spritesheet(TexturePtr, frame_w, frame_h, attrs)` (which assumes every frame is the same size and arranged in a regular grid), the `Spritesheet` asset stores an explicit rectangle per frame. This lets it represent sheets that were **tightly packed** by an atlas-generation tool, where frames can be different sizes and are not aligned to a grid.

A `Spritesheet` also carries any named animations the atlas defines (a name plus a start/end frame range), which are automatically registered on a `Sprite` when you call `set_spritesheet(SpritesheetPtr)`.

## 2. The Atlas JSON Format

The loader accepts the widely-used **TexturePacker "array" JSON atlas format**. This is the same format written by the [`sprite_gen`](../guides/sprite-gen.md) tool's `--atlas` option, and by most other atlas/packer tools (TexturePacker, Aseprite, Free Texture Packer, etc. all support exporting this format).

```json
{
    "frames": [
        {
            "filename": "walk_0",
            "frame": {"x": 2, "y": 2, "w": 60, "h": 64},
            "rotated": false,
            "trimmed": false,
            "sourceSize": {"w": 60, "h": 64},
            "spriteSourceSize": {"x": 0, "y": 0, "w": 60, "h": 64},
            "duration": 66
        },
        {
            "filename": "walk_1",
            "frame": {"x": 66, "y": 2, "w": 60, "h": 64},
            "rotated": false,
            "trimmed": false,
            "sourceSize": {"w": 60, "h": 64},
            "spriteSourceSize": {"x": 0, "y": 0, "w": 60, "h": 64},
            "duration": 66
        }
    ],
    "meta": {
        "image": "walk.tga",
        "size": {"w": 128, "h": 128},
        "scale": "1",
        "frameTags": [
            {"name": "walk", "from": 0, "to": 1, "direction": "forward"}
        ]
    }
}
```

### Fields Simulant Reads

| Field | Description |
|-------|-------------|
| `frames[].filename` | The frame's name. Looked up with `Spritesheet::find_frame()`. |
| `frames[].frame.x/y/w/h` | The frame's pixel rectangle on the sheet. |
| `frames[].duration` | Frame duration in **milliseconds**. Used to derive the FPS of any animation that includes this frame. |
| `meta.image` | The sheet texture's filename, resolved **relative to the atlas JSON file**. |
| `meta.frameTags[].name/from/to` | A named animation covering frames `from` to `to` (inclusive), by **index into the `frames` array** -- not pixel coordinates. |

### Fields Simulant Ignores

`rotated`, `trimmed`, `sourceSize`, `spriteSourceSize`, `meta.scale`, and `frameTags[].direction` are accepted (they won't cause a parse error) but are not currently interpreted. In particular: **rotated and trimmed frames are not supported** -- frames must be stored unrotated, and `frame.w`/`frame.h` are used directly as the visible size.

### Frame Indices

Frames are indexed by their **position in the `frames` array**, starting at 0 -- exactly like the existing grid-based sprite sheet frame numbering used by `Sprite::add_animation()`. `frameTags[].from`/`to` refer to these same indices.

## 3. Loading a Spritesheet

```cpp
auto sheet = assets->load_spritesheet("sprites/hero.json");
if (!sheet) {
    S_ERROR("Failed to load spritesheet");
    return;
}
```

`load_spritesheet()` follows the same conventions as the other `AssetManager::load_*()` methods:

```cpp
SpritesheetPtr load_spritesheet(
    const Path& filename,
    GarbageCollectMethod garbage_collect = GARBAGE_COLLECT_PERIODIC,
    bool use_asset_cache = true
);
```

- The atlas path is resolved through the VFS, same as any other asset.
- The referenced `meta.image` texture is loaded automatically (its containing folder is temporarily added to the search path so relative image paths resolve correctly).
- By default, loading the same atlas path twice from the same `AssetManager` returns the already-loaded `Spritesheet` (`use_asset_cache`).

If the file can't be found, isn't valid JSON, is missing `frames`/`meta`, or `meta.image` can't be loaded, `load_spritesheet()` logs an error and returns a null `SpritesheetPtr`.

## 4. The Spritesheet API

```cpp
class Spritesheet {
public:
    TexturePtr texture() const;

    std::size_t frame_count() const;
    const SpritesheetFrame* frame(std::size_t i) const;
    optional<std::size_t> find_frame(const std::string& name) const;

    std::size_t animation_count() const;
    const SpritesheetAnimation* animation(std::size_t i) const;
};

struct SpritesheetFrame {
    std::string name;
    uint16_t x, y, w, h;
    uint32_t duration_ms;
};

struct SpritesheetAnimation {
    std::string name;
    uint32_t start_frame;
    uint32_t end_frame;
};
```

```cpp
auto sheet = assets->load_spritesheet("sprites/hero.json");

S_INFO("Loaded {0} frames, {1} animations",
       sheet->frame_count(), sheet->animation_count());

// Look up a frame by name
if (auto idx = sheet->find_frame("walk_3")) {
    auto* frame = sheet->frame(idx.value());
    S_INFO("walk_3 is at ({0}, {1}), {2}x{3}", frame->x, frame->y, frame->w, frame->h);
}

// Inspect the animations defined in the atlas
for (std::size_t i = 0; i < sheet->animation_count(); ++i) {
    auto* anim = sheet->animation(i);
    S_INFO("Animation '{0}': frames {1}-{2}", anim->name, anim->start_frame, anim->end_frame);
}
```

## 5. Using a Spritesheet with Sprite

### Setting It After Creation

```cpp
auto sheet = assets->load_spritesheet("sprites/hero.json");

auto sprite = create_child<Sprite>();
sprite->set_spritesheet(sheet);
sprite->set_render_dimensions(1.0f, 1.0f);

// Any frameTags in the atlas are automatically registered as animations,
// using the per-frame `duration` from the atlas to derive the FPS.
sprite->animations->play_animation("walk");
```

### Setting It at Creation Time

`Sprite` also accepts a `"spritesheet"` parameter through the `Params` system, so you can wire it up in one call:

```cpp
auto sheet = assets->load_spritesheet("sprites/hero.json");

auto sprite = create_child<Sprite>(Params().set("spritesheet", sheet));
sprite->set_render_dimensions(1.0f, 1.0f);
sprite->animations->play_animation("walk");
```

This parameter is optional -- `create_child<Sprite>()` with no parameters still works exactly as before, and you can call `set_spritesheet()` later.

### Adding Your Own Animations

Atlas-defined `frameTags` are just a convenience -- you can still call `add_animation()` yourself (inherited from `KeyFrameAnimated`) to define additional animations, or override ones from the atlas, using the same frame indices:

```cpp
sprite->set_spritesheet(sheet);

// Add an animation that isn't in the atlas, or override the derived FPS
sprite->add_animation("walk_fast", 0, 1, 20.0f);
```

## 6. Spritesheet vs. the Grid-Based API

| | `set_spritesheet(TexturePtr, w, h, attrs)` | `set_spritesheet(SpritesheetPtr)` |
|---|---|---|
| Frame layout | Uniform grid, calculated from frame width/height, margin, spacing, padding | Explicit per-frame rectangle |
| Frame sizes | All frames must be the same size | Frames can be different sizes |
| Source | You configure it in code | Loaded from a JSON atlas file |
| Animation names | You call `add_animation()` yourself | Auto-registered from `frameTags`, or add your own on top |
| Typical origin | Hand-built sheets, simple tilesets | Sheets from `sprite_gen`, TexturePacker, Aseprite, etc. |

Both approaches configure the same underlying `Sprite` machinery (a quad mesh + alpha-blended material), so everything else -- `set_render_dimensions()`, `flip_horizontally()`/`flip_vertically()`, `set_alpha()`, `animations->play_animation()` -- works identically regardless of which one you use.

---

## See Also

- [Sprites](../rendering/sprites.md) -- General 2D sprite rendering guide
- [Sprite Animation](../animation/sprite-animation.md) -- Playing and controlling animations
- [sprite_gen Tool](../guides/sprite-gen.md) -- Generate a spritesheet + atlas from a 3D model
- [Asset Managers](asset-managers.md) -- General asset loading and lifetime rules
- [JSON Parsing](../utilities/json.md) -- The parser used internally to read atlas files
