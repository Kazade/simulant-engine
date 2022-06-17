#define RND_IMPLEMENTATION
#include "_rnd.h"
#include "random.h"

namespace smlt {

RandomGenerator::RandomGenerator() {
    time_t seconds;
    time(&seconds);
    rnd_pcg_seed(&rand_, (RND_U32) seconds);
}

RandomGenerator::RandomGenerator(uint32_t seed) {
    rnd_pcg_seed(&rand_, (RND_U32) seed);
}

float RandomGenerator::float_in_range(float lower, float upper) {
    float dist = upper - lower;
    return lower + (dist * rnd_pcg_nextf(&rand_));
}

int32_t RandomGenerator::int_in_range(int32_t lower, int32_t upper) {
    return rnd_pcg_range(&rand_, lower, upper);
}

Vec2 RandomGenerator::point_in_circle(float diameter) {
    return point_on_circle(diameter) * float_in_range(0.0f, 1.0f);
}

Vec3 RandomGenerator::point_in_sphere(float diameter) {
    // Find a random point on the edge (vector) then scale between 0 and 1
    return point_on_sphere(diameter) * float_in_range(0.0f, 1.0f);
}

Vec2 RandomGenerator::point_on_circle(float diameter) {
    auto x = float_in_range(-1.0f, 1.0f);
    auto y = float_in_range(-1.0f, 1.0f);
    return smlt::Vec2(x, y).normalized() * diameter;
}

Vec3 RandomGenerator::point_on_sphere(float diameter) {
    auto x = float_in_range(-1.0f, 1.0f);
    auto y = float_in_range(-1.0f, 1.0f);
    auto z = float_in_range(-1.0f, 1.0f);
    return smlt::Vec3(x, y, z).normalized() * diameter;
}

Vec2 RandomGenerator::direction_2d() {
    return point_on_circle(1.0f);
}

Vec3 RandomGenerator::direction_3d() {
    return point_on_sphere(1.0f);
}

}
