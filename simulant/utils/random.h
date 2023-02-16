#pragma once

#include <random>
#include <vector>
#include "../types.h"

#include <cstdint>

#define RND_U32 uint32_t
#define RND_U64 uint64_t

#include "_rnd.h"

namespace smlt {

class RandomGenerator {
public:
    RandomGenerator();
    RandomGenerator(uint32_t seed);

    template<typename T>
    T choice(T* array, std::size_t count) {
        return array[int_in_range(0, count - 1)];
    }

    template<typename T>
    T choice(const std::vector<T>& choices) {
        return this->choice<T>((T*) &choices[0], choices.size());
    }

    template<typename T>
    void shuffle(T* array, std::size_t count);

    template<typename T>
    void shuffle(std::vector<T>& choices);

    template<typename T>
    std::vector<T> shuffled(const std::vector<T>& choices);

    float float_in_range(float lower, float upper);
    int32_t int_in_range(int32_t lower, int32_t upper);
    smlt::Vec2 point_in_circle(float diameter);
    smlt::Vec3 point_in_sphere(float diameter);
    smlt::Vec2 point_on_circle(float diameter);
    smlt::Vec3 point_on_sphere(float diameter);
    smlt::Vec2 direction_2d();
    smlt::Vec3 direction_3d();

private:
    rnd_pcg_t rand_;
};


}
