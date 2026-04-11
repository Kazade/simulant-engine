# Sound File Formats

Simulant supports loading sound assets in two formats: **OGG Vorbis** and **WAV**. Both are loaded through the asset manager like any other asset, but they differ in how they are handled internally and which platforms support them.

## Loading Sounds

Sounds are loaded using the asset manager's `load_sound(path)` method:

```cpp
auto sound = assets->load_sound("sounds/explosion.wav");
```

You can also pass loading flags to customise behaviour:

```cpp
smlt::SoundLoadFlags sound_flags;
// Configure flags as needed
auto sound = assets->load_sound("sounds/music.ogg", sound_flags);
```

## OGG Vorbis

OGG Vorbis is a compressed, streaming audio format that is ideal for longer audio content such as background music and ambient soundscapes.

### Characteristics

| Property | Value |
|----------|-------|
| **Extension** | `.ogg` |
| **Compression** | Yes (lossy) |
| **Streaming** | Yes - streamed from disk |
| **File size** | Small (compressed) |
| **Memory usage** | Low (streamed, not fully loaded) |

### When to Use OGG

Use OGG for audio that is:

- **Long in duration** - Background music, ambient loops, long voice lines
- **Memory-constrained** - OGG streams from disk rather than loading entirely into RAM
- **Repeated frequently** - Smaller file size means faster asset loading

```cpp
// Stream background music
auto music = assets->load_sound("music/forest.ogg");
music_node->play_sound(music, smlt::AUDIO_REPEAT_FOREVER, smlt::DISTANCE_MODEL_AMBIENT);
```

### Platform Support

| Platform | OGG Support |
|----------|-------------|
| **Linux** | Yes |
| **Windows** | Yes |
| **Dreamcast** | Yes |
| **PSP** | **No** |
| **Android** | Yes |
| **macOS** | Yes |

> **Important:** OGG is **not supported on PSP**. Use WAV for PSP projects.

## WAV

WAV is an uncompressed audio format that stores raw PCM sample data. It is ideal for short sound effects that need to play with minimal latency.

### Characteristics

| Property | Value |
|----------|-------|
| **Extension** | `.wav` |
| **Compression** | No (uncompressed PCM) |
| **Streaming** | No - fully loaded into memory |
| **File size** | Larger (uncompressed) |
| **Memory usage** | Higher (entire file loaded) |
| **Playback latency** | Lower (no decoding overhead) |

### When to Use WAV

Use WAV for audio that is:

- **Short in duration** - Sound effects, UI clicks, short impacts
- **Latency-sensitive** - Uncompressed audio plays back instantly
- **Played infrequently** - Memory cost is acceptable for occasional sounds

```cpp
// Play short sound effects
auto explosion = assets->load_sound("sfx/explosion.wav");
auto click = assets->load_sound("sfx/ui_click.wav");
auto jump = assets->load_sound("sfx/jump.wav");
```

### Platform Support

| Platform | WAV Support |
|----------|-------------|
| **Linux** | Yes |
| **Windows** | Yes |
| **Dreamcast** | Yes |
| **PSP** | Yes |
| **Android** | Yes |
| **macOS** | Yes |

WAV is the most universally supported format across all Simulant platforms.

## Internal Audio Data Formats

Under the hood, Simulant represents decoded audio using the `AudioDataFormat` enum. The supported formats are:

| Format | Description |
|--------|-------------|
| `AUDIO_DATA_FORMAT_MONO8` | Mono, 8-bit |
| `AUDIO_DATA_FORMAT_MONO16` | Mono, 16-bit |
| `AUDIO_DATA_FORMAT_MONO24` | Mono, 24-bit |
| `AUDIO_DATA_FORMAT_STEREO8` | Stereo, 8-bit |
| `AUDIO_DATA_FORMAT_STEREO16` | Stereo, 16-bit |
| `AUDIO_DATA_FORMAT_STEREO24` | Stereo, 24-bit |

You can query a loaded sound's format:

```cpp
auto sound = assets->load_sound("sfx/explosion.wav");
auto format = sound->format();
auto bytes = smlt::audio_data_format_byte_size(format);
S_DEBUG("Sound format: {}, bytes per sample: {}", format, bytes);
```

## Choosing the Right Format

Here is a practical guide for deciding which format to use:

| Sound Type | Recommended Format | Reason |
|------------|-------------------|--------|
| **Background Music** | OGG | Large files benefit from compression and streaming |
| **Ambient Loops** | OGG | Continuous playback benefits from small memory footprint |
| **Explosions / Impacts** | WAV | Short duration, needs instant playback |
| **UI Sounds** | WAV | Very short, latency matters |
| **Voice Lines (short)** | WAV | Quick response needed |
| **Voice Lines (long)** | OGG | Memory savings for longer dialogue |
| **Environmental Sounds** | OGG | Can be longer ambient recordings |

## Platform-Specific Considerations

### Dreamcast

Both OGG and WAV are supported on Dreamcast. However, Dreamcast has limited RAM (typically 16MB), so prefer OGG for any non-trivial audio content to avoid exhausting available memory.

```cpp
// On Dreamcast, prefer OGG for music to conserve RAM
auto music = assets->load_sound("music/level.ogg");
```

### PSP

The PSP build does **not** support OGG audio. All sounds must be in WAV format:

```cpp
// PSP only - WAV required
auto music = assets->load_sound("music/level.wav");
auto sfx = assets->load_sound("sfx/explosion.wav");
```

For PSP projects, consider keeping WAV files as short as possible to manage memory usage, since they are fully loaded into RAM.

### Desktop (Linux / Windows / macOS)

Both formats work reliably on all desktop platforms. Use the guidelines above to choose based on content type rather than platform constraints.

## Asset Pipeline Recommendations

### Converting Audio

Use [Audacity] (free, open-source) or your preferred audio editor to convert between formats:

- **Export as OGG Vorbis** for music and ambient audio (quality setting 5-7 is usually sufficient for games)
- **Export as WAV (16-bit PCM)** for sound effects

### File Organization

A typical project audio structure:

```
assets/
├── sounds/
│   ├── music/
│   │   ├── level_theme.ogg
│   │   ├── boss_fight.ogg
│   │   └── menu_music.ogg
│   └── sfx/
│       ├── explosion.wav
│       ├── jump.wav
│       ├── ui_click.wav
│       └── ambient/
│           ├── forest_loop.ogg
│           └── cave_drip.wav
```

### Memory Management

Sounds are managed as assets and are reference-counted. When a sound is no longer referenced, it is automatically unloaded:

```cpp
{
    auto sound = assets->load_sound("sfx/temp.wav");
    node->play_sound(sound);
    // sound goes out of scope, but stays alive because PlayingSound holds a reference
}
// Once playback finishes, the sound is automatically cleaned up if no other references exist
```

You can also explicitly destroy sounds:

```cpp
assets->destroy_sound(sound->id());
sound.reset();
```

## Troubleshooting

### Sound plays silently or not at all

- Verify the file path is correct relative to your assets directory
- Check that the file format is supported on your target platform (no OGG on PSP)
- Ensure the `AudioSource` node is in the scene and not destroyed prematurely
- Verify the audio listener is configured on the `Window`

### High memory usage from audio

- Switch long or looped sounds from WAV to OGG
- Reduce the sample rate of sound effects (22050 Hz is often sufficient for SFX)
- Use `destroy_sound` to explicitly unload sounds that are no longer needed

### Audio stuttering or gaps in loops

- Ensure your OGG loop points are set correctly in the source file
- Use `AUDIO_REPEAT_FOREVER` for seamless looping
- Consider pre-trimming silence from the start and end of looped audio files

[Audacity]: https://www.audacityteam.org/
