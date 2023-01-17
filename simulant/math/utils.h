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

/** Sum of Squares (w^2 + x^2 + y^2 + z^2) */
float fast_sum_of_squares(float x, float y, float z, float w);

/** Same as fminf */
float fast_min(float a, float b);

/** Same as fmaxf */
float fast_max(float a, float b);

/** Returns 1 / sqrt(n) */
float fsrra(float n);

/** Returns the inner (dot) product (x1 * x2) + (y1 * y2) + (z1 * z2) * (w1 * w2) */
float fipr(float x1, float y1, float z1, float w1, float x2, float y2, float z2, float w2);


/** Clamps x between l and h*/
inline float clamp(const float x, const float l, const float h) {
    return fast_min(fast_max(x, l), h);
}

/** Linear interpolation from x to y with factor t, where t can be any value between 0 and 1 */
inline float lerp(const float x, const float y, const float t) {
    return fast_fmaf((y - x), t, x);
}
}
