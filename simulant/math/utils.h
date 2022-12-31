#pragma once

#include <cmath>
#include <limits>
#include <cstdint>
#include <algorithm>

namespace smlt {

const float PI = std::acos(-1.0f);
const float TWO_PI = PI * 2.0f;
const float PI_OVER_180 = PI / 180.0f;
const float PI_UNDER_180 = 180.0f / PI;
const float EPSILON = std::numeric_limits<float>::epsilon();

inline float clamp(const float x, const float l, const float h) {
    return std::min(std::max(x, l), h);
}

inline float lerp(const float x, const float y, const float t) {
    return fmaf((y - x), t, x);
}

float smoothstep(const float e0, const float e1, float x);
float smootherstep(const float e0, const float e1, float x);

template<typename T>
bool almost_equal(const T& lhs, const T& rhs) {
    const T EPSILON = std::numeric_limits<T>::epsilon();
    return lhs + EPSILON > rhs &&
           lhs - EPSILON < rhs;
}

template<typename T>
bool almost_lequal(const T& lhs, const T& rhs, const T& epsilon) {
    return lhs <= (rhs + epsilon);
}

template<typename T>
bool almost_gequal(const T& lhs, const T& rhs, const T& epsilon) {
    return lhs >= (rhs - epsilon);
}

uint32_t next_power_of_two(uint32_t x);

/** Approximate divide which is faster on some platforms */
float fast_divide(float x, float y);
float fast_sqrt(float n);
void fast_sincos(double v, double* s, double* c);

}
