#include <algorithm>
#include <cassert>
#include <chrono>
#include <cmath>
#include <numeric>
#include "../types.h"
#include "../math/lerp.h"

#include "noise.h"

#define PI 3.14159265358979323846f
#define PIOver180  (PI / 180.0f)

namespace smlt {
namespace noise {

static float fade(float t) {
    return t * t * t * (t * (t * 6.0f - 15.0f) + 10.0f);
}

static float grad(int hash, float x, float y, float z) {
    int h = hash & 15;
    float u = h < 8 ? x : y;
    float v = h < 4 ? y : h == 12 || h == 14 ? x : z;
    return ((h & 1) == 0 ? u : -u) + ((h & 2) == 0 ? v : -v);
}

Perlin::Perlin(uint32_t seed) {
    if(!seed) {
        seed = std::chrono::system_clock::now().time_since_epoch().count();
    }

    auto mid_range = p.begin() + 256;

    std::mt19937 engine(seed);

    std::iota(p.begin(), mid_range, 0); //Generate sequential numbers in the lower half
    std::shuffle(p.begin(), mid_range, engine); //Shuffle the lower half
    std::copy(p.begin(), mid_range, mid_range); //Copy the lower half to the upper half
    //p now has the numbers 0-255, shuffled, and duplicated

    float r = 3.1f * PIOver180; //Arbitrary 3.1 degrees in radians
    rotation_ = Quaternion(Radians(r), Radians(r), Radians(r)); //Create a quaternion rotated 3.1 degrees around each axis
}

float Perlin::noise(float x, float y, float z) const {
    //FIXME: Use rotation_ to rotate the coordinates

    //See here for algorithm: http://cs.nyu.edu/~perlin/noise/

    const int32_t X = static_cast<int32_t>(std::floor(x)) & 255;
    const int32_t Y = static_cast<int32_t>(std::floor(y)) & 255;
    const int32_t Z = static_cast<int32_t>(std::floor(z)) & 255;

    x -= std::floor(x);
    y -= std::floor(y);
    z -= std::floor(z);

    const float u = fade(x);
    const float v = fade(y);
    const float w = fade(z);

    const auto A = p[X] + Y;
    const auto AA = p[A] + Z;
    const auto AB = p[A + 1] + Z;
    const auto B = p[X + 1] + Y;
    const auto BA = p[B] + Z;
    const auto BB = p[B + 1] + Z;

    const auto PAA = p[AA];
    const auto PBA = p[BA];
    const auto PAB = p[AB];
    const auto PBB = p[BB];
    const auto PAA1 = p[AA + 1];
    const auto PBA1 = p[BA + 1];
    const auto PAB1 = p[AB + 1];
    const auto PBB1 = p[BB + 1];

    const auto a = smlt::lerp(
        smlt::lerp(grad(PAA, x, y, z), grad(PBA, x-1, y, z), u),
        smlt::lerp(grad(PAB, x, y-1, z), grad(PBB, x-1, y-1, z), u),
        v
    );

    const auto b = smlt::lerp(
        smlt::lerp(grad(PAA1, x, y, z-1), grad(PBA1, x-1, y, z-1), u),
        smlt::lerp(grad(PAB1, x, y-1, z-1), grad(PBB1, x-1, y-1, z-1), u),
        v
    );

    return smlt::lerp(a, b, w);
}

uint32_t FractalNoise::resolve_seed(uint32_t seed) {
    if(!seed) {
        seed = std::chrono::system_clock::now().time_since_epoch().count();
        // Perlin treats 0 as "pick a seed", so make sure we never pass it on
        if(!seed) {
            seed = 1;
        }
    }
    return seed;
}

FractalNoise::FractalNoise(int octaves, uint32_t seed, float lacunarity, float persistence):
    seed_(resolve_seed(seed)),
    perlin_(seed_),
    octaves_(std::max(octaves, 1)),
    lacunarity_(lacunarity),
    persistence_(persistence) {

    /* Generate a distinct, non-integer offset per octave. Without this every
     * octave samples the same lattice scaled about the origin, so features
     * line up around (0, 0, 0) and the noise is always 0 there. Use a
     * separate engine (derived from the seed) to the permutation shuffle so
     * the two don't correlate. */
    std::mt19937 engine(seed_ ^ 0x9E3779B9u);

    /* mt19937's raw output is fully specified by the standard, whereas
     * uniform_real_distribution isn't, so map it to [0, 256) by hand to keep
     * offsets identical across standard libraries. */
    auto dist = [](std::mt19937& e) -> float {
        // Use the top 24 bits so the result is exactly representable as a float
        return float(e() >> 8) * (256.0f / 16777216.0f);
    };

    offsets_.resize(octaves_);
    for(auto& offset: offsets_) {
        offset = {dist(engine), dist(engine), dist(engine)};
    }

    float amp = 1.0f;
    amplitude_sum_ = 0.0f;
    for(int i = 0; i < octaves_; ++i) {
        amplitude_sum_ += amp;
        amp *= persistence_;
    }

    if(amplitude_sum_ <= 0.0f) {
        amplitude_sum_ = 1.0f;
    }
}

float FractalNoise::noise(float x, float y, float z) const {
    float result = 0.0f;
    float amp = 1.0f;
    float freq = 1.0f;

    for(int i = 0; i < octaves_; ++i) {
        const auto& o = offsets_[i];
        result += perlin_.noise(
            x * freq + o[0],
            y * freq + o[1],
            z * freq + o[2]
        ) * amp;

        freq *= lacunarity_;
        amp *= persistence_;
    }

    // Clamp to guard against float rounding pushing us fractionally outside -1..1
    return std::min(1.0f, std::max(-1.0f, result / amplitude_sum_));
}

}
}
