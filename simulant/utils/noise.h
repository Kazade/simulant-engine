#ifndef NOISE_H
#define NOISE_H

#include <random>
#include <array>
#include "../math/quaternion.h"

namespace smlt {
namespace noise {

class Perlin {
public:
    Perlin(uint32_t seed=0);

    double noise(double x) const { return noise(x, 0, 0); }
    double noise(double x, double y) const { return noise(x, y, 0); }
    double noise(double x, double y, double z) const;

    double normalized_noise(double x) {
        return (noise(x) + 1.0) * 0.5;
    }

    double normalized_noise(double x, double y) {
        return (noise(x, y) + 1.0) * 0.5;
    }

    double normalized_noise(double x, double y, double z) {
        return (noise(x, y, z) + 1.0) * 0.5;
    }

private:
    std::array<int, 512> p;
    Quaternion rotation_;
};

class PerlinOctave {
public:
    PerlinOctave(int octaves, uint32_t seed=0);

    double noise(double x) const { return noise(x, 0, 0); }
    double noise(double x, double y) const { return noise(x, y, 0); }
    double noise(double x, double y, double z) const;

    double normalized_noise(double x) {
        return (noise(x) + 1.0) * 0.5;
    }

    double normalized_noise(double x, double y) {
        return (noise(x, y) + 1.0) * 0.5;
    }

    double normalized_noise(double x, double y, double z) {
        return (noise(x, y, z) + 1.0) * 0.5;
    }

private:
    Perlin perlin_;
    int octaves_;
};

}
}

#endif // NOISE_H
