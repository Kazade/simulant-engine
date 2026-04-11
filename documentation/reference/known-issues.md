# Known Issues & TODO

This page tracks current limitations, known issues, and planned improvements for the Simulant engine.

## Current Limitations

### Rendering

- **Maximum Light Count**: Platform-dependent light limits (Dreamcast: 2 lights, PSP: 4 lights, Desktop: unlimited)
- **Texture Size Limits**: Maximum texture size varies by platform (Dreamcast: 1024px, PSP: 512px power-of-two only)
- **No Real-time Global Illumination**: Lighting is direct only; no baked or real-time GI
- **Shadow Maps**: Limited shadow map support; no cascaded shadow maps

### Physics

- **Collision Layers**: Simple bit-flag based filtering; no complex collision matrices
- **Joint Types**: Limited to sphere joints; no hinge, slider, or 6DOF joints yet
- **Soft Bodies**: No soft body or cloth physics simulation

### Audio

- **3D Sound**: Basic positional audio with distance attenuation; no HRTF or advanced spatialization
- **Streaming**: Large audio files are streamed, but seek performance can be improved

### Platform-Specific

#### Dreamcast

- OGG Vorbis decoding not supported; WAV only due to RAM constraints
- Custom shaders not supported (GLdc uses fixed-function pipeline)
- Asset embedding into ELF required; no file system access

#### PSP

- OGG format not supported; WAV only
- Textures must be power-of-two and max 512px
- Audio subsystem partially implemented

## TODO / Planned Features

### High Priority

- [ ] Improved shadow system (cascaded shadow maps)
- [ ] Post-processing effects pipeline
- [ ] GPU instancing improvements
- [ ] Better mobile/device support

### Medium Priority

- [ ] Additional physics joint types (hinge, slider, 6DOF)
- [ ] Vehicle physics
- [ ] Particle system editor tool
- [ ] Asset pipeline improvements

### Low Priority

- [ ] Vulkan renderer backend
- [ ] WebAssembly export
- [ ] Networking/multiplayer support
- [ ] Advanced animation blending

## Reporting Issues

If you encounter a bug or limitation not listed here, please report it on the [GitLab Issues](https://gitlab.com/simulant/simulant/-/issues) page.

## Contributing

We welcome contributions! See [CONTRIBUTING.md](../../CONTRIBUTING.md) for information on how to help implement these features.
