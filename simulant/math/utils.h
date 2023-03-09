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

/** Approximate divide which is faster on some platforms */
float fast_divide(float x, float y);

/** Fast square root */
float fast_sqrt(float n);

/** Fast sine cosine (double) */
void fast_sincos(double v, double* s, double* c);

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

/** Clamps x between l and h*/
__attribute__((optimize("O3", "fast-math")))
inline float clamp(const float x, const float l, const float h) {
#ifdef __DREAMCAST__
    return __builtin_fminf(__builtin_fmaxf(x, l), h);
#else
    return fast_min(fast_max(x, l), h);
#endif
}

/** Linear interpolation from x to y with factor t, where t can be any value between 0 and 1 */
__attribute__((optimize("O3", "fast-math")))
inline float lerp(const float x, const float y, const float t) {
#ifdef __DREAMCAST__
    return __builtin_fmaf((y - x), t, x);
#else
    return fast_fmaf((y - x), t, x);
#endif
}

__attribute__((optimize("O3", "fast-math")))
inline float fast_divide(float d, float n) {
#ifdef __DREAMCAST__
    const float sgn = ((*(unsigned int*)&n)>>31)*2+1;
    return sgn * (1.f / __builtin_sqrtf(n * n)) * d;
#else
    return d / n;
#endif
}

__attribute__((optimize("O3", "fast-math")))
inline float fast_sqrt(float n) {
    return __builtin_sqrtf(n);
}

__attribute__((optimize("O3", "fast-math")))
inline void fast_sincos(float v, float* s, float* c) {
#ifdef __DREAMCAST__
    __builtin_sincosf(v, s, c);
#else
    *s = sinf(v);
    *c = cosf(v);
#endif
}

__attribute__((optimize("O3", "fast-math")))
inline float fast_abs(float x) {
#ifdef __DREAMCAST__
    return __builtin_fabsf(x);
#else
    return std::abs(x);
#endif
}

__attribute__((optimize("O3", "fast-math")))
inline float fast_fmaf(float a, float b, float c) {
#ifdef __DREAMCAST__
    return __builtin_fmaf(a, b, c);
#else
    return fmaf(a, b, c);
#endif
}

__attribute__((optimize("O3", "fast-math")))
inline float fast_min(float a, float b) {
#ifdef __DREAMCAST__
    return __builtin_fminf(a, b);
#else
    return std::min(a, b);
#endif
}

__attribute__((optimize("O3", "fast-math")))
inline float fast_max(float a, float b) {
#ifdef __DREAMCAST__
    return __builtin_fmaxf(a, b);
#else
    return std::max(a, b);
#endif
}

__attribute__((optimize("O3", "fast-math")))
inline float fast_inverse_sqrt(float n) {
    /* On Dreamcast this is enough to optimise to fsrra */
    return 1 / std::sqrt(n);
}

}
