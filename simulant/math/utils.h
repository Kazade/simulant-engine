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

/* fast_ prefixed versions of functions allow of loss of precision over speed. IT IS IMPORTANT
 THAT THESE ARE USED WHEN COMPILED WITH -O3 -ffast-math or these will be drastically slower */
static inline float fast_divide(float d, float n) {
#ifdef __DREAMCAST__
#ifndef NDEBUG
    /* Non release builds use d / n because the alternative is soooo much slower if you're
     * compiling without optimisations */
    return d / n;
#else
    /* On Dreamcast this is enough to optimise to fsrra and some bitshifts */
    union {
        float f;
        uint32_t i;
    } c;
    c.f = n;
    const float r = (1.f / std::sqrt(c.f * c.f)) * d;
    const uint32_t sgn = (c.i >> 31) * 2 + 1;
    return r * sgn;
#endif

#else
    return d / n;
#endif
}

static inline float fast_sqrt(float n) {
    return std::sqrt(n);
}

static inline void fast_sincos(float v, float* s, float* c) {
#ifdef __DREAMCAST__
    __builtin_sincosf(v, s, c);
#else
    *s = std::sin(v);
    *c = std::cos(v);
#endif
}

static inline float fast_abs(float x) {
    return std::abs(x);
}

static inline float fast_fmaf(float a, float b, float c) {
    return ::fmaf(a, b, c);
}

static inline float fast_min(float a, float b) {
    return std::min(a, b);
}

static inline float fast_max(float a, float b) {
    return std::max(a, b);
}

static inline float fast_clamp(const float x, const float l, const float h) {
    return std::min(std::max(x, l), h);
}

static inline float fast_inverse_sqrt(float n) {
    /* On Dreamcast this is enough to optimise to fsrra */
    return 1 / std::sqrt(n);
}

}
