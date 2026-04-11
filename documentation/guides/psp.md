# PSP (PlayStation Portable) Development Guide

This guide covers the specifics of developing for the PlayStation Portable (PSP) with Simulant, including hardware limitations, build setup, asset constraints, and optimization strategies.

---

## Table of Contents

1. [Hardware Overview](#1-hardware-overview)
2. [Build Setup](#2-build-setup)
3. [Memory Constraints](#3-memory-constraints)
4. [Graphics Limitations](#4-graphics-limitations)
5. [Audio on PSP](#5-audio-on-psp)
6. [Asset Embedding](#6-asset-embedding)
7. [File System and VFS](#7-file-system-and-vfs)
8. [Input and Controls](#8-input-and-controls)
9. [Debugging](#9-debugging)
10. [Optimization Checklist](#10-optimization-checklist)
11. [Distribution](#11-distribution)

---

## 1. Hardware Overview

The PSP is a handheld gaming device with moderate resources -- more capable than the Dreamcast but still significantly constrained compared to desktop platforms.

### Specifications

| Component | Specification |
|-----------|--------------|
| **CPU** | MIPS R4000 @ 333MHz |
| **RAM** | 32MB main memory |
| **GPU** | Custom PSP graphics processor |
| **Display** | 480x272 LCD (16:9 aspect ratio) |
| **Audio** | Hardware audio (WAV support) |
| **Storage** | UMD disc, Memory Stick (homebrew) |
| **Input** | D-pad, analog nub, face buttons, shoulder buttons |

### Renderer

Simulant uses a **custom PSP renderer** rather than OpenGL. The PSP GPU has its own command-based rendering API that differs significantly from desktop OpenGL:

- Fixed-function rendering (similar to OpenGL ES 1.x)
- No custom shaders
- Hardware-transformed and lit (HT&L) geometry
- Texture swizzling required for optimal performance

### Max 4 Lights Per Object

The PSP renderer supports up to **4 lights per renderable**. This is more than Dreamcast (2) but less than desktop (8).

---

## 2. Build Setup

### Prerequisites

Building for PSP requires the PSP homebrew toolchain (pspsdk). The recommended approach is using **Docker** with the Simulant PSP SDK image.

### Building

```bash
# Build for PSP
simulant build psp

# This uses Docker to cross-compile in a controlled environment
```

### Compiler Considerations

The PSP toolchain uses a MIPS-based GCC cross-compiler. Key considerations:

- **No RTTI** -- Runtime type information is disabled
- **No exceptions** -- Exception handling is disabled
- **No standard threading** -- Use Simulant's threading subsystem (`simulant/thread`) instead of `std::thread`

See the [C++ Guidelines](../reference/cpp-guidelines.md) for details on embedded platform restrictions.

### Debug vs Release

```bash
# Debug build
simulant build psp --debug

# Release build
simulant build psp --release
```

---

## 3. Memory Constraints

### 32MB RAM

The PSP has **32MB of RAM**, double the Dreamcast's 16MB. This provides more headroom but still requires careful memory management.

**Practical limits:**
- Keep total asset footprint under **20-24MB**
- Textures remain the largest memory consumer
- The 32MB is shared between system and graphics

### Memory Management

The same strategies used for Dreamcast apply to PSP, with somewhat more breathing room:

**1. Use appropriately sized textures:**

```cpp
// PSP: max 512px, power-of-two required
auto tex = assets->load_texture("textures/hero_256.png");
```

**2. Garbage collection between scenes:**

```cpp
void on_deactivate() override {
    level_mesh_id_ = smlt::AssetID();
    assets->run_garbage_collection();
}
```

**3. Store AssetIDs, not shared_ptrs:**

```cpp
class Player {
    MeshID mesh_id_;  // Correct
    // MeshPtr mesh_; // Wrong -- prevents GC
};
```

---

## 4. Graphics Limitations

### Texture Requirements

**This is the most critical PSP constraint:**

| Requirement | Value |
|-------------|-------|
| **Maximum texture size** | **512 pixels** |
| **Power-of-two** | **Required** (256x256, 512x512, 512x256, etc.) |

**Non-power-of-two textures will fail to load or render incorrectly on PSP.**

### Valid Texture Sizes

| Width | Valid Heights |
|-------|--------------|
| 64 | 64, 128, 256, 512 |
| 128 | 64, 128, 256, 512 |
| 256 | 64, 128, 256, 512 |
| 512 | 64, 128, 256, 512 |

### Texture Format

On PSP, textures are loaded from PNG/JPG files and converted to the PSP's native texture format at load time. There is no pre-compressed GPU texture format like DDS on desktop or DTEX on Dreamcast.

```cpp
// Load from PNG -- converted at load time
auto tex = assets->load_texture("textures/hero.png");

// Set filtering
tex->set_texture_filter(smlt::TEXTURE_FILTER_BILINEAR);
```

### No Custom Shaders

Like Dreamcast, the PSP does not support custom shaders. All rendering uses fixed-function pipeline.

### Lighting

The PSP supports up to **4 lights per renderable**. This allows more complex lighting than Dreamcast:

```cpp
// On PSP: ambient + up to 3 dynamic lights per object
lighting->set_ambient_light(smlt::Color(0.3f, 0.3f, 0.4f, 1.0f));

auto sun = create_child<smlt::DirectionalLight>();
sun->set_intensity(1.0f);

// You can add point lights too (up to 4 total per object)
auto torch = create_child<smlt::PointLight>();
torch->set_intensity(0.5f);
torch->transform->set_position(5, 3, 0);
```

### Screen Resolution

The PSP screen is **480x272** pixels (16:9 widescreen). Set your window and UI accordingly:

```cpp
smlt::AppConfig config;
config.title = "My PSP Game";
config.width = 480;
config.height = 272;
config.fullscreen = true;
```

---

## 5. Audio on PSP

### Supported Formats

| Format | Extension | Notes |
|--------|-----------|-------|
| **WAV** | `.wav` | Supported. Loaded into memory. |
| **OGG Vorbis** | `.ogg` | **Not supported** on PSP. |

### Audio Status

Audio on PSP is currently **not fully implemented** in Simulant. WAV files may work but functionality is limited. Plan accordingly:

- Test audio thoroughly on actual hardware
- Provide fallback visual feedback for audio events
- Monitor Simulant development for improved PSP audio support

### Using WAV

```cpp
#ifdef SIMULANT_PLATFORM_PSP
    // Use WAV for all audio on PSP
    auto music = assets->load_sound("music/level.wav");
    auto sfx = assets->load_sound("sfx/jump.wav");
#else
    // Use OGG for music on other platforms
    auto music = assets->load_sound("music/level.ogg");
    auto sfx = assets->load_sound("sfx/jump.wav");
#endif
```

---

## 6. Asset Embedding

Like Dreamcast, the PSP has no traditional filesystem for homebrew applications. Simulant **embeds all assets into the executable** during the build process.

### Configuring Assets

```json
{
    "name": "mygame",
    "target_platforms": ["linux", "psp"],
    "asset_paths": ["assets"],
    "core_assets": true
}
```

### Binary Size

The PSP ELF must fit within available memory. Keep the binary under **20MB** for safety.

---

## 7. File System and VFS

### Virtual File System Paths

On PSP, the VFS supports these path prefixes:

| Prefix | Description |
|--------|-------------|
| `./` | Current directory |
| `umd0:` | UMD disc drive |
| `ms0:` | Memory Stick |
| `disc0:` | Disc device |

### Platform-Specific Assets

Organize PSP-specific assets in subdirectories:

```
assets/
├── models/
│   ├── character.glb        # Desktop version
│   └── psp/
│       └── character.glb    # PSP-optimized (fewer bones, lower poly)
└── textures/
    ├── ground.png           # Desktop (512x512)
    └── psp/
        └── ground.png       # PSP (256x256, power-of-two)
```

The VFS automatically tries PSP-specific paths first.

### Conditional Code

```cpp
#ifdef SIMULANT_PLATFORM_PSP
    // PSP-specific code
    // - Power-of-two texture enforcement
    // - WAV-only audio
    // - Reduced polygon counts
    // - Lower physics frequency
#endif
```

---

## 8. Input and Controls

### PSP Controls

The PSP has these input devices:

| Control | Simulant Key Code |
|---------|------------------|
| D-pad Up | `KEYBOARD_CODE_UP` |
| D-pad Down | `KEYBOARD_CODE_DOWN` |
| D-pad Left | `KEYBOARD_CODE_LEFT` |
| D-pad Right | `KEYBOARD_CODE_RIGHT` |
| Cross (X) | Game-specific mapping |
| Circle (O) | Game-specific mapping |
| Square ([]) | Game-specific mapping |
| Triangle | Game-specific mapping |
| L Shoulder | Game-specific mapping |
| R Shoulder | Game-specific mapping |
| Analog Nub | Analog input |
| Start | Game-specific mapping |
| Select | Game-specific mapping |

### Input Handling

```cpp
void handle_input(float dt) {
    // D-pad movement
    smlt::Vec2 movement(0, 0);
    if (input->key_pressed(smlt::KEYBOARD_CODE_UP)) movement.y += 1;
    if (input->key_pressed(smlt::KEYBOARD_CODE_DOWN)) movement.y -= 1;
    if (input->key_pressed(smlt::KEYBOARD_CODE_LEFT)) movement.x -= 1;
    if (input->key_pressed(smlt::KEYBOARD_CODE_RIGHT)) movement.x += 1;

    // Action button
    if (input->key_just_pressed(smlt::KEY_SPACE)) {  // Mapped to Cross
        player_jump();
    }

    // Menu/back button
    if (input->key_just_pressed(smlt::KEY_ESCAPE)) {  // Mapped to Circle
        scenes->activate("menu");
    }
}
```

---

## 9. Debugging

### Logging

Use `S_INFO` and `S_DEBUG` for logging. Output behavior on PSP depends on the development environment:

```cpp
S_INFO("Scene loaded with {} actors", actor_count);
S_DEBUG("Texture loaded: {}", texture_path);
```

### Profiling

Simulant's profiler works on PSP:

```bash
# Enable profiling in config
config.development.force_profiling = true;

// In code:
S_PROFILE_SECTION("Frame Update");
// ... update code ...
S_PROFILE_DUMP_TO_STDOUT();
```

### Emulators

Test your game in PSP emulators before deploying to real hardware:

- **PPSSPP** -- The most widely used PSP emulator. Available on desktop and mobile.
- Test both homebrew loading and UMD image formats.

---

## 10. Optimization Checklist

Use this checklist when preparing a PSP build:

### Textures
- [ ] **ALL textures are power-of-two dimensions** -- non-negotiable
- [ ] No texture exceeds **512px** in either dimension
- [ ] Use 256x256 where possible to save VRAM
- [ ] Test every texture on actual PSP hardware or PPSSPP

### Models
- [ ] Use PSP-optimized models (lower poly than desktop)
- [ ] Reduce bone counts in skeletal animations
- [ ] Keep on-screen triangle count reasonable

### Lighting
- [ ] Maximum **4 lights per object**
- [ ] Use ambient + 1-2 dynamic lights for most scenes
- [ ] Test lighting on actual hardware

### Audio
- [ ] **Use WAV format only** -- OGG is not supported
- [ ] Audio may not be fully functional -- test thoroughly
- [ ] Provide visual fallbacks for audio cues

### Memory
- [ ] Total binary size under **20MB**
- [ ] Run garbage collection between scenes
- [ ] Preload assets to avoid runtime hitches

### Display
- [ ] Window size set to **480x272**
- [ ] UI elements sized appropriately for PSP screen
- [ ] Text readable at PSP resolution

### Physics
- [ ] Use simple collider shapes
- [ ] Consider reducing physics to **30Hz** if CPU-bound
- [ ] Avoid triangle mesh colliders for dynamic objects

### Code
- [ ] No `std::thread` -- use Simulant's threading
- [ ] No RTTI or exceptions (disabled by toolchain)
- [ ] Use `#ifdef SIMULANT_PLATFORM_PSP` for platform-specific code

---

## 11. Distribution

### Building a PSP Package

```bash
simulant package psp
```

This generates a PSP-compatible package in the `packages/` directory.

### Homebrew Distribution

For homebrew distribution:
- Place the EBOOT file and assets on a Memory Stick
- Structure: `ms0:/PSP/GAME/mygame/EBOOT.PBP`
- Users can launch from the PSP's Game menu

### Testing Distribution

Before releasing:
1. Test in **PPSSPP** emulator for basic functionality
2. Test on **real PSP hardware** if possible
3. Verify all textures load correctly (power-of-two check)
4. Verify audio plays (if implemented)
5. Check memory usage does not cause crashes

---

## Summary

PSP development in Simulant requires attention to:

1. **32MB RAM** -- more than Dreamcast, but still constrained
2. **Power-of-two textures, max 512px** -- the most critical constraint
3. **4 lights per object** -- more than Dreamcast, fewer than desktop
4. **WAV audio only** -- OGG not supported; audio may be partially implemented
5. **Embedded assets** -- all content compiled into the ELF
6. **480x272 display** -- widescreen aspect ratio
7. **No RTTI or exceptions** -- toolchain restrictions
8. **No custom shaders** -- fixed-function rendering only

The PSP offers a good balance between constraint and capability. It is more forgiving than Dreamcast while still providing the challenge and satisfaction of optimizing for a retro platform.

---

## Further Reading

- [Asset Pipeline Guide](asset-pipeline.md) -- Asset preparation and formats
- [Performance Guide](performance.md) -- Optimization techniques
- [3D Game Development Guide](3d-games.md) -- 3D development best practices
- [Dreamcast Development Guide](dreamcast.md) -- Similar constrained platform
- [Platform Notes](../reference/platform_notes.md) -- Platform API overview
- [C++ Guidelines](../reference/cpp-guidelines.md) -- Coding standards for embedded targets
