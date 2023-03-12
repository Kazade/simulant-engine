#pragma once

#include <cmath>
#include <limits>
#include <cstdint>
#include <algorithm>

#ifdef __DREAMCAST__
#include "../utils/sh4_math.h"
#endif

namespace smlt {

const float PI = std::acos(-1.0f);
const float TWO_PI = PI * 2.0f;
const float PI_OVER_180 = PI / 180.0f;
const float PI_UNDER_180 = 180.0f / PI;
const float EPSILON = std::numeric_limits<float>::epsilon();

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


/** Clamps x between l and h*/
inline float clamp(const float x, const float l, const float h) {
    return std::min(std::max(x, l), h);
}

/** Linear interpolation from x to y with factor t, where t can be any value between 0 and 1 */
inline float lerp(const float x, const float y, const float t) {
    return ::fmaf((y - x), t, x);
}

/* fast_ prefixed versions of functions allow of loss of precision over speed */
float fast_clamp(const float x, const float l, const float h);

/** Approximate divide which is faster on some platforms */
float fast_divide(float x, float y);

/** Fast square root */
float fast_sqrt(float n);

/** Fast sine cosine */
void fast_sincos(float v, float* s, float* c);

/** Fast absolute*/
float fast_abs(float x);

/** Returns a*b+c */
float fast_fmaf(float a, float b, float c);

/** Same as fminf */
float fast_min(float a, float b);

/** Same as fmaxf */
float fast_max(float a, float b);

/** Returns the inverse square root: 1 / sqrt(n) */
float fast_inverse_sqrt(float n);

__attribute__((optimize("O3", "fast-math")))
inline float fast_divide(float d, float n) {
#ifdef __DREAMCAST__
    union {
        float f;
        uint32_t i;
    } c;
    c.f = n;
    const float r = (1.f / std::sqrt(c.f * c.f)) * d;
    const uint32_t sgn = (c.i >> 31) * 2 + 1;
    return r * sgn;
#else
    return d / n;
#endif
}

__attribute__((optimize("O3", "fast-math")))
inline float fast_sqrt(float n) {
    return std::sqrt(n);
}

__attribute__((optimize("O3", "fast-math")))
inline void fast_sincos(float v, float* s, float* c) {
#ifdef __DREAMCAST__
    __builtin_sincosf(v, s, c);
#else
    *s = std::sin(v);
    *c = std::cos(v);
#endif
}

__attribute__((optimize("O3", "fast-math")))
inline float fast_abs(float x) {
    return std::abs(x);
}

__attribute__((optimize("O3", "fast-math")))
inline float fast_fmaf(float a, float b, float c) {
    return ::fmaf(a, b, c);
}

__attribute__((optimize("O3", "fast-math")))
inline float fast_min(float a, float b) {
    return std::min(a, b);
}

__attribute__((optimize("O3", "fast-math")))
inline float fast_max(float a, float b) {
    return std::max(a, b);
}

__attribute__((optimize("O3", "fast-math")))
inline float fast_clamp(const float x, const float l, const float h) {
    return std::min(std::max(x, l), h);
}

__attribute__((optimize("O3", "fast-math")))
inline float fast_inverse_sqrt(float n) {
    /* On Dreamcast this is enough to optimise to fsrra */
    return 1 / std::sqrt(n);
}

}
