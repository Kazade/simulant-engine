#include "random.h"
#include "../application.h"

#include <cstdint>
#include <random>

namespace smlt {

struct _RandomImpl {
    std::mt19937 engine;

    _RandomImpl(uint32_t seed) :
        engine(seed) {}
};

RandomGenerator::~RandomGenerator() = default;

RandomGenerator::RandomGenerator() :
    impl_(std::make_unique<_RandomImpl>(std::random_device()())) {}

RandomGenerator::RandomGenerator(uint32_t seed) :
    impl_(std::make_unique<_RandomImpl>(seed)) {}

float RandomGenerator::float_in_range(float lower, float upper) {
    std::uniform_real_distribution<> distr(lower, upper);
    return distr(impl_->engine);
}

int32_t RandomGenerator::int_in_range(int32_t lower, int32_t upper) {
    std::uniform_int_distribution<> distr(lower, upper);
    return distr(impl_->engine);
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
