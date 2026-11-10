#ifndef NOISE_H
#define NOISE_H

#include <random>
#include <array>
#include <vector>
#include "../math/quaternion.h"

namespace smlt {
namespace noise {

class Perlin {
public:
    Perlin(uint32_t seed=0);

    float noise(float x) const { return noise(x, 0, 0); }
    float noise(float x, float y) const { return noise(x, y, 0); }
    float noise(float x, float y, float z) const;

    float normalized_noise(float x) const {
        return (noise(x) + 1.0f) * 0.5f;
    }

    float normalized_noise(float x, float y) const {
        return (noise(x, y) + 1.0f) * 0.5f;
    }

    float normalized_noise(float x, float y, float z) const {
        return (noise(x, y, z) + 1.0f) * 0.5f;
    }

private:
    std::array<int, 512> p;
    Quaternion rotation_;
};

/**
 * Fractal (fBm) noise built from multiple octaves of Perlin noise.
 *
 * Each successive octave is sampled at `lacunarity` times the frequency
 * and `persistence` times the amplitude of the previous one.
 *
 * The result is divided by the sum of the octave amplitudes, so noise()
 * stays within the same range as single-octave Perlin (roughly -1..1)
 * regardless of the octave count, and normalized_noise() stays in 0..1.
 */
class FractalNoise {
public:
    FractalNoise(
        int octaves,
        uint32_t seed=0,
        float lacunarity=2.0f,
        float persistence=0.5f
    );

    float noise(float x) const { return noise(x, 0, 0); }
    float noise(float x, float y) const { return noise(x, y, 0); }
    float noise(float x, float y, float z) const;

    float normalized_noise(float x) const {
        return (noise(x) + 1.0f) * 0.5f;
    }

    float normalized_noise(float x, float y) const {
        return (noise(x, y) + 1.0f) * 0.5f;
    }

    float normalized_noise(float x, float y, float z) const {
        return (noise(x, y, z) + 1.0f) * 0.5f;
    }

    int octaves() const { return octaves_; }
    float lacunarity() const { return lacunarity_; }
    float persistence() const { return persistence_; }

private:
    static uint32_t resolve_seed(uint32_t seed);

    // Must be declared before perlin_ so it's initialized first
    uint32_t seed_;
    Perlin perlin_;
    int octaves_;
    float lacunarity_;
    float persistence_;
    float amplitude_sum_ = 1.0f;
    std::vector<std::array<float, 3>> offsets_;
};

}
}

#endif // NOISE_H
