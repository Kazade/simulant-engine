# Noise

Simulant provides gradient noise functions for procedural generation: terrain, biomes, clouds, texture variation, camera shake and so on.

| Class | Header | Dimensions | Use it for |
|-------|--------|------------|------------|
| `smlt::noise::Perlin` | `<simulant/utils/noise.h>` | 1D, 2D, 3D | A single layer of smooth noise |
| `smlt::noise::FractalNoise` | `<simulant/utils/noise.h>` | 1D, 2D, 3D | Natural-looking detail (terrain, biomes, clouds) |
| `smlt::Simplex` | `<simulant/utils/simplex.h>` | 2D, 3D, 4D | Single-layer noise in 4D, e.g. looping 3D animation |

None of these headers are included by `simulant.h`, so include them explicitly.

---

## Perlin

`Perlin` is classic (improved) Perlin noise.

```cpp
#include <simulant/utils/noise.h>

using namespace smlt::noise;

Perlin perlin(1234);   // Pass 0 (the default) to seed from the clock

float a = perlin.noise(x);          // 1D
float b = perlin.noise(x, y);       // 2D
float c = perlin.noise(x, y, z);    // 3D

// Same, but remapped from -1..1 to 0..1
float d = perlin.normalized_noise(x, y);
```

- `noise()` returns values in the range -1..1. `normalized_noise()` returns 0..1.
- **Perlin noise is always 0 at whole-number coordinates.** Scale your inputs (e.g. `x * 0.05f`) rather than passing raw integer grid positions, or you'll get a flat result.
- The pattern repeats every 256 units of input.

---

## FractalNoise

`FractalNoise` adds several layers ("octaves") of Perlin noise together. Each octave adds finer detail at a smaller amplitude. This is also known as fractal Brownian motion (fBm), and it's the usual building block for terrain and biome maps.

```cpp
#include <simulant/utils/noise.h>

using namespace smlt::noise;

// FractalNoise(octaves, seed=0, lacunarity=2.0f, persistence=0.5f)
FractalNoise terrain(6, 1234);

float h = terrain.noise(x * 0.01f, z * 0.01f);             // -1..1
float h01 = terrain.normalized_noise(x * 0.01f, z * 0.01f); // 0..1
```

### Parameters

| Parameter | Default | Effect |
|-----------|---------|--------|
| `octaves` | — | Number of layers. More octaves add finer detail but cost more: each octave is one Perlin sample. Values below 1 are treated as 1. |
| `seed` | `0` | Seeds the noise. `0` picks a seed from the clock. |
| `lacunarity` | `2.0f` | Frequency multiplier between octaves. Higher values make each octave's detail much finer than the last. |
| `persistence` | `0.5f` | Amplitude multiplier between octaves. Lower values give smoother results, higher values give rougher ones. |

You can read these back with `octaves()`, `lacunarity()` and `persistence()`.

### Behaviour

- **The output is normalised.** The sum of the octaves is divided by the total amplitude, so `noise()` stays within -1..1 and `normalized_noise()` within 0..1 for any combination of parameters. Changing the octave count changes the detail, not the range.
- **Each octave is offset.** Each octave samples the noise at a different, seed-derived offset. As a result, octaves don't line up at the origin, and unlike plain `Perlin` the result isn't zero at whole-number coordinates.
- **It's deterministic.** The same seed and parameters always produce the same output.

### Example: Minecraft-style biomes

Biomes are usually picked by combining two or more independent, low-frequency noise fields, such as temperature and humidity. Give each field its own seed so they don't match each other.

```cpp
#include <simulant/utils/noise.h>

using namespace smlt::noise;

enum class Biome { TUNDRA, TAIGA, PLAINS, FOREST, DESERT, JUNGLE };

class WorldGenerator {
public:
    WorldGenerator(uint32_t seed):
        temperature_(4, seed),
        humidity_(4, seed + 1),
        height_(6, seed + 2) {}

    Biome biome_at(int x, int z) const {
        // A low frequency gives large biomes
        const float scale = 0.002f;

        float t = temperature_.noise(x * scale, z * scale);
        float h = humidity_.noise(x * scale, z * scale);

        if(t < -0.3f) return h > 0.0f ? Biome::TAIGA : Biome::TUNDRA;
        if(t >  0.3f) return h > 0.0f ? Biome::JUNGLE : Biome::DESERT;
        return h > 0.0f ? Biome::FOREST : Biome::PLAINS;
    }

    int height_at(int x, int z) const {
        // A higher frequency than the biome maps gives hills and valleys
        const float scale = 0.02f;
        return 64 + int(height_.noise(x * scale, z * scale) * 24.0f);
    }

private:
    FractalNoise temperature_;
    FractalNoise humidity_;
    FractalNoise height_;
};
```

Tips:

- **Smooth biome borders.** To avoid cliffs where biomes meet, store terrain parameters (base height, amplitude) per biome. Then interpolate them using the raw temperature and humidity values rather than switching abruptly at the thresholds.
- **Make the terrain less uniform.** A third low-frequency "continentalness" or "erosion" field, used to scale `height_at()`, gives a mix of flat areas and mountains.
- **Mind the repeat.** Like `Perlin`, the base octave repeats every 256 units of input. At `scale = 0.002f` that's 128,000 blocks, which is plenty for most worlds.

### Precision

The inputs are `float`. Far from the origin (around ±100,000 world units and beyond), the gap between neighbouring floats becomes large enough that a small scale factor makes the noise look blocky. If your world is that big, keep noise coordinates relative to a nearby origin, e.g. per chunk or region.

---

## Simplex

`Simplex` is simplex noise in 2D, 3D and 4D. It's reference counted, so create it with `Simplex::create()`.

```cpp
#include <simulant/utils/simplex.h>

auto simplex = smlt::Simplex::create(42);   // Seeds from time(NULL) if omitted

float n2 = simplex->noise(x, y);
float n3 = simplex->noise(x, y, z);
float n4 = simplex->noise(x, y, z, w);      // Roughly -1..1
```

- The output is roughly -1..1. There's no `normalized_noise()`, so remap it yourself with `(n + 1.0f) * 0.5f`.
- `Simplex` has no octave support. If you need fractal detail in 2D or 3D, use `FractalNoise` instead.
- 4D noise is handy for animating 3D noise smoothly: use time as the fourth coordinate.
- **Seeding caveats:** `Simplex` shuffles its permutation table with `srand()`/`rand()`. This reseeds the global C random generator, and it also means the same seed can produce different noise on different platforms. If you need identical output on every platform from one seed (e.g. shared world seeds), use `Perlin` or `FractalNoise`.

---

## Performance

Noise is relatively expensive, particularly on Dreamcast and PSP. A `FractalNoise` sample costs roughly one `Perlin` sample per octave.

- Generate noise at load time, or when a chunk is created, and store the results in a heightmap, texture or block array. Avoid sampling every frame.
- Use only as many octaves as you can see. Detail finer than a single block or pixel is wasted work.
- Low-frequency fields like biome maps can be sampled on a coarse grid and interpolated.
