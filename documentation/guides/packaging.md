# Packaging and Distribution Guide

This guide covers how to build, package, and distribute a Simulant game for all supported platforms. It covers the build process, packaging commands, platform-specific output formats, and distribution best practices.

---

## Table of Contents

1. [Overview](#1-overview)
2. [Building Your Game](#2-building-your-game)
3. [Packaging for Distribution](#3-packaging-for-distribution)
4. [Platform-Specific Packages](#4-platform-specific-packages)
5. [Asset Embedding](#5-asset-embedding)
6. [Cross-Platform Builds with Docker](#6-cross-platform-builds-with-docker)
7. [Distribution Formats by Platform](#7-distribution-formats-by-platform)
8. [Version Management](#8-version-management)
9. [Distribution Best Practices](#9-distribution-best-practices)
10. [Troubleshooting](#10-troubleshooting)

---

## 1. Overview

Simulant provides a unified build and packaging system through the `simulant` command-line tool. This tool handles:

- Compiling your C++ code for target platforms
- Embedding assets into executables (for constrained platforms)
- Generating distributable packages
- Managing project configuration

### The Packaging Workflow

```
Source Code + Assets --> simulant build --> Binary + Assets --> simulant package --> Distributable
```

---

## 2. Building Your Game

### Local Build

For your native platform (typically Linux or Windows):

```bash
simulant build
```

This compiles your code and produces an executable in the platform-specific build directory (e.g., `build/linux/mygame`).

### Running Your Build

```bash
# Run the built game
simulant run

# Run and rebuild in one command
simulant run --rebuild
```

### Build Types

```bash
# Debug build (with symbols, slower)
simulant build --debug

# Release build (optimized, stripped)
simulant build --release
```

### Platform-Specific Builds

If you have Docker installed, you can cross-compile for other platforms:

```bash
# Windows build (requires Windows SDK Docker image)
simulant build windows

# Dreamcast build (requires Dreamcast SDK Docker image)
simulant build dreamcast

# PSP build (requires PSP SDK Docker image)
simulant build psp
```

---

## 3. Packaging for Distribution

### Basic Packaging

```bash
# Package for all configured target platforms
simulant package

# Package for a specific platform
simulant package linux
simulant package windows
simulant package dreamcast
simulant package psp
```

Packages are generated in the `packages/` directory of your project.

### What Gets Packaged

The packaging process:
1. Compiles your code for the target platform
2. Embeds assets (for platforms without a filesystem)
3. Generates the distributable output format (ELF, CDI, etc.)
4. Places output in `packages/`

---

## 4. Platform-Specific Packages

### Linux

**Output:** Native ELF executable + assets directory

```bash
simulant package linux
```

**Distribution:**
- Share the executable from `build/linux/` alongside the `assets/` directory
- Create a tarball for easy distribution:

```bash
tar czf mygame-v1.0.0-linux.tar.gz \
    build/linux/mygame \
    assets/
```

**Launch script:** Optionally provide a shell script:

```bash
#!/bin/bash
cd "$(dirname "$0")"
./mygame "$@"
```

### Windows

**Output:** Windows executable (.exe) + assets

```bash
simulant build windows
simulant package windows
```

**Distribution:**
- Zip the executable and assets folder together
- The executable is statically linked where possible (MinGW builds use `-static-libgcc -static-libstdc++`)

```bash
# Create a distributable zip (on Linux, using zip)
zip -r mygame-v1.0.0-windows.zip \
    build/windows/mygame.exe \
    assets/
```

### Dreamcast

**Output:** CDI disc image (or .bin file)

```bash
simulant package dreamcast
```

**Distribution:**
- The generated `.cdi` file can be:
  - Burned to a CD-R for real hardware
  - Loaded in Dreamcast emulators (lxdream, Redream, etc.)
  - Distributed online for homebrew channels

```bash
# Test in emulator
simulant run dreamcast
```

**CD Burning:** Use any standard CD burning software to burn the CDI image to a CD-R. The Dreamcast reads standard CD-R media.

### PSP

**Output:** PSP homebrew package

```bash
simulant package psp
```

**Distribution:**
- Place on Memory Stick in the homebrew structure:
  ```
  ms0:/PSP/GAME/mygame/
  ├── EBOOT.PBP
  └── (embedded assets are in the EBOOT)
  ```
- Users launch from the PSP Game menu

---

## 5. Asset Embedding

### How Embedding Works

For platforms without a traditional filesystem (Dreamcast, PSP), Simulant embeds all assets directly into the executable during the build process:

1. The build system scans all directories listed in `asset_paths`
2. Each file is converted to a C data array
3. These arrays are compiled into the final ELF executable
4. At runtime, the Virtual File System (VFS) serves these embedded assets

### Desktop Platforms

On desktop (Linux, Windows), assets are **not embedded** -- they are loaded from disk at runtime. This means:
- Faster iteration during development (edit assets, restart game)
- The distributed package must include both the executable AND the assets folder
- Users can mod/replace assets easily

### Constrained Platforms

On Dreamcast and PSP, assets are **always embedded** -- there is no option to load from external files. This means:
- The executable contains everything needed to run
- No separate assets folder to distribute
- Users cannot easily mod assets

### Configuring Asset Paths

Ensure your `simulant.json` includes all asset directories:

```json
{
    "name": "mygame",
    "version": "1.0.0",
    "description": "My awesome game",
    "author": "Your Name",
    "target_platforms": ["linux", "dreamcast", "psp"],
    "asset_paths": ["assets"],
    "core_assets": true
}
```

Multiple asset paths:

```json
{
    "asset_paths": ["assets", "shared-assets", "dlc"]
}
```

### Verifying Embedded Assets

After building for Dreamcast or PSP, check the binary size:

```bash
ls -lh packages/mygame_dreamcast.*
```

A surprisingly small binary may indicate missing assets. Verify all asset paths in `simulant.json`.

---

## 6. Cross-Platform Builds with Docker

### Required Docker Images

For cross-compilation, you need the appropriate SDK Docker images:

```bash
# Dreamcast SDK
docker pull kazade/dreamcast-sdk

# Windows SDK
docker pull kazade/windows-sdk

# PSP SDK (if available as a separate image)
```

### Docker Requirements

- Docker must be installed and running
- Your user must be in the `docker` group:

```bash
sudo usermod -aG docker $USER
# Log out and back in for this to take effect
```

### Building in Docker

When you run `simulant build dreamcast`, the tool:
1. Starts a Docker container with the appropriate SDK
2. Mounts your project directory into the container
3. Runs the build inside the container
4. Outputs the result back to your host filesystem

First-time builds are slow because the Docker image must be downloaded. Subsequent builds are faster.

---

## 7. Distribution Formats by Platform

| Platform | Output Format | Assets | Notes |
|----------|--------------|--------|-------|
| **Linux** | ELF executable | Separate directory | Run with `./mygame` |
| **Windows** | `.exe` | Separate directory | May need OpenAL/SDL2 DLLs |
| **Dreamcast** | `.cdi` / `.bin` | Embedded | Burn to CD-R or use in emulator |
| **PSP** | Homebrew package | Embedded | Place in `PSP/GAME/` on Memory Stick |
| **macOS** | Executable | Separate directory | Untested regularly |
| **Android** | APK | Embedded/packaged | Via Android toolchain |

### Runtime Dependencies

**Linux:** The executable may depend on:
- `libsdl2`
- `libopenal`

These are typically available on modern Linux distributions. Users may need to install them:

```bash
# Fedora
sudo dnf install SDL2 openal-soft

# Ubuntu/Debian
sudo apt install libsdl2-2.0-0 libopenal1
```

**Windows:** The MinGW build statically links the C/C++ runtime, but may need:
- `OpenAL32.dll`
- `SDL2.dll`

Include these DLLs alongside your executable, or instruct users to install them.

---

## 8. Version Management

### Versioning Your Game

Set the version in `simulant.json`:

```json
{
    "name": "mygame",
    "version": "1.0.0",
    "description": "My awesome game",
    "author": "Your Name"
}
```

Use [semantic versioning](https://semver.org/):
- **MAJOR.MINOR.PATCH**
- Increment MAJOR for incompatible changes
- Increment MINOR for new features
- Increment PATCH for bug fixes

### Naming Packages

Include version and platform in package filenames:

```
mygame-1.0.0-linux.tar.gz
mygame-1.0.0-windows.zip
mygame-1.0.0-dreamcast.cdi
mygame-1.0.0-psp.zip
```

---

## 9. Distribution Best Practices

### Before Releasing

1. **Test on all target platforms** -- Do not assume what works on desktop works everywhere
2. **Verify asset paths** -- Ensure `simulant.json` lists all necessary `asset_paths`
3. **Check binary sizes** -- Unrealistically small binaries may indicate missing assets
4. **Run in release mode** -- Debug builds may hide performance issues
5. **Test from a clean state** -- Delete `build/` and rebuild from scratch

### Documentation

Include with your release:
- **README** -- What the game is, how to run it, known issues
- **Controls** -- Button mappings, keyboard controls
- **Credits** -- Acknowledge assets, libraries, and contributors
- **License** -- Your game's license (if applicable)

### Online Distribution

**For desktop platforms:**
- Upload to itch.io, Game Jolt, or your own website
- Provide separate packages for Linux and Windows
- Include a changelog for updates

**For Dreamcast:**
- Upload the CDI image to homebrew hosting sites
- Provide instructions for burning or emulating
- Include screenshots and a description

**For PSP:**
- Distribute as a homebrew package
- Users place files on Memory Stick via USB
- Test in PPSSPP and on real hardware

### Update Distribution

When releasing updates:
1. Increment the version in `simulant.json`
2. Rebuild and repackage for all platforms
3. Name packages with the new version
4. Provide a changelog listing changes

---

## 10. Troubleshooting

### Build Failures

| Problem | Solution |
|---------|----------|
| `simulant build` fails on native platform | Check that SDL2 and OpenAL development packages are installed |
| Docker build fails | Ensure Docker is running and your user is in the `docker` group |
| Missing asset errors | Verify `asset_paths` in `simulant.json` includes all asset directories |
| Linker errors | Run `simulant update` to ensure engine libraries are current |

### Runtime Issues

| Problem | Solution |
|---------|----------|
| Game crashes on startup | Check that assets are in the correct location relative to the executable |
| Textures not loading | Verify file paths are correct and case-sensitive (Linux) |
| Audio not playing | Ensure OpenAL is installed and working |
| Low FPS | Profile with `SIMULANT_PROFILE=1` to identify bottlenecks |

### Platform-Specific Issues

| Problem | Platform | Solution |
|---------|----------|----------|
| Texture not rendering | PSP | Check that texture is power-of-two and under 512px |
| OGG music not playing | PSP | Use WAV format instead |
| Binary too large | Dreamcast/PSP | Reduce texture sizes, remove unused assets |
| Model appears wrong | Dreamcast | Use Dreamcast-specific low-poly model variant |

---

## Summary

Key steps for packaging and distributing a Simulant game:

1. **Configure `simulant.json`** with correct `target_platforms` and `asset_paths`
2. **Build for each platform** using `simulant build [platform]`
3. **Package using `simulant package [platform]`** to generate distributables
4. **Test the packaged output** on each target platform
5. **Name packages with version and platform** for clarity
6. **Include documentation** (README, controls, credits)
7. **Distribute through appropriate channels** for each platform

---

## Further Reading

- [Project Structure](../getting-started/project-structure.md) -- Understanding your project layout
- [Asset Pipeline Guide](asset-pipeline.md) -- Preparing assets for all platforms
- [Performance Guide](performance.md) -- Optimization before release
- [Dreamcast Guide](dreamcast.md) -- Dreamcast-specific constraints
- [PSP Guide](psp.md) -- PSP-specific constraints
- [Building a Complete Game](complete-game.md) -- Step-by-step game tutorial
