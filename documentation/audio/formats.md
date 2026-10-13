# Sound File Formats

Simulant supports loading sound assets in three formats: **OGG Vorbis**, **WAV** and **ADX**. All are loaded through the asset manager like any other asset, but they differ in how they are handled internally and which platforms support them.

## Loading Sounds

Sounds are loaded using the asset manager's `load_sound(path)` method:

```cpp
auto sound = assets->load_sound("sounds/explosion.wav");
```

You can also pass loading flags to customise behaviour:

```cpp
smlt::SoundFlags sound_flags;
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
| **PSP** | Yes |
| **Android** | Yes |
| **macOS** | Yes |

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

## ADX

ADX is a lightweight ADPCM format originally created by CRI for the Dreamcast. It stores 4-bit samples in fixed-size blocks, which makes it roughly a quarter of the size of 16-bit WAV while being very cheap to decode. On Dreamcast the inner decode loop is hand-written SH4 assembly.

### Characteristics

| Property | Value |
|----------|-------|
| **Extension** | `.adx` |
| **Compression** | Yes (lossy, about 4:1 against 16-bit PCM) |
| **Streaming** | Yes - streamed from disk by default |
| **File size** | Medium (larger than OGG, much smaller than WAV) |
| **Memory usage** | Low when streamed |
| **Channels** | Mono or stereo |

### When to Use ADX

Use ADX for audio where decoding cost matters more than file size:

- **Music on Dreamcast** - Decoding takes far less CPU time than OGG Vorbis
- **Frequently played sound effects** - Load into memory to keep them compressed in RAM
- **Long voice lines** - Streams from disk like OGG

```cpp
// Stream background music (the default)
auto music = assets->load_sound("music/forest.adx");
music_node->play_sound(music, smlt::AUDIO_REPEAT_FOREVER, smlt::DISTANCE_MODEL_AMBIENT);
```

### Streaming vs Loading into Memory

ADX sounds are streamed from disk unless you ask otherwise:

```cpp
smlt::SoundFlags sfx_flags;
sfx_flags.stream_audio = false;
auto jump = assets->load_sound("sfx/jump.adx", sfx_flags);
```

- **Streaming** opens the file again for each playing instance of the sound. Platforms with a low limit on open files (e.g. Dreamcast) can run out if many streamed sounds play at once.
- **Loading into memory** reads the file once and keeps it compressed. Every playing instance shares that copy and decodes it as it plays, so no extra files are opened.

### Limitations

- Files should be encoded with ffmpeg (see [Converting Audio](#converting-audio)). Files produced by CRI's own tools will load, but play back with reduced quality.
- Loop points stored in the file are ignored. Use `AUDIO_REPEAT_FOREVER` to loop, which restarts from the beginning.
- Encrypted files and files with more than two channels are not supported.

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
| **Music on Dreamcast** | ADX | Much cheaper to decode than OGG, still streamed |

## Platform-Specific Considerations

### Dreamcast

OGG, WAV and ADX are all supported on Dreamcast. However, Dreamcast has limited RAM (typically 16MB), so prefer a streamed, compressed format for any non-trivial audio content to avoid exhausting available memory. ADX was designed for the Dreamcast and costs much less CPU time to decode than OGG, at the price of larger files.

```cpp
// On Dreamcast, prefer ADX or OGG for music to conserve RAM
auto music = assets->load_sound("music/level.adx");
```

### PSP

OGG and WAV are both supported on PSP. With 32MB of RAM, prefer OGG for music so that it is streamed rather than held in memory:

```cpp
auto music = assets->load_sound("music/level.ogg");
auto sfx = assets->load_sound("sfx/explosion.wav");
```

### Desktop (Linux / Windows / macOS)

Both formats work reliably on all desktop platforms. Use the guidelines above to choose based on content type rather than platform constraints.

## Asset Pipeline Recommendations

### Converting Audio

[ffmpeg] converts between all of the supported formats from the command line. The input can be anything ffmpeg can read (MP3, OGG, WAV, FLAC...), these commands only differ in the output.

**To ADX:**

```bash
ffmpeg -i input.mp3 -c:a adpcm_adx output.adx
```

**To OGG Vorbis** (quality 5-7 is usually sufficient for games):

```bash
ffmpeg -i input.mp3 -c:a libvorbis -q:a 5 output.ogg
```

**To WAV (16-bit PCM):**

```bash
ffmpeg -i input.mp3 -c:a pcm_s16le output.wav
```

Add these options before the output filename to shrink the result:

| Option | Effect |
|--------|--------|
| `-ar 22050` | Resample to 22050 Hz, which halves the size. Often sufficient for sound effects |
| `-ac 1` | Mix down to mono, which halves the size of a stereo file. Positional sounds should be mono |
| `-ac 2` | Mix down to stereo. Required for ADX if the source has more than two channels |

For example, to turn a stereo OGG into a mono 22050 Hz ADX sound effect:

```bash
ffmpeg -i explosion.ogg -c:a adpcm_adx -ar 22050 -ac 1 explosion.adx
```

When converting from OGG to ADX, ffmpeg may print a long run of `non monotonically increasing dts` warnings. The output is unaffected. Adding `-af asetpts=N/SR/TB` silences them:

```bash
ffmpeg -i input.ogg -af asetpts=N/SR/TB -c:a adpcm_adx output.adx
```

To convert a whole directory:

```bash
for f in *.wav; do
    ffmpeg -i "$f" -c:a adpcm_adx "${f%.wav}.adx"
done
```

ADX, OGG and MP3 are all lossy, so convert from the original recording where you can rather than from another lossy file.

If you prefer a graphical tool, [Audacity] (free, open-source) can export OGG Vorbis and 16-bit PCM WAV.

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
- Check that the file extension is one of the supported formats (`.ogg`, `.wav`, `.adx`)
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
[ffmpeg]: https://ffmpeg.org/
