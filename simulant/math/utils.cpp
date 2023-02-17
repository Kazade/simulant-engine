#include "utils.h"

namespace smlt {

float smoothstep(const float e0, const float e1, float x) {
    x = clamp(fast_divide((x - e0), (e1 - e0)), 0.0f, 1.0f);
    return x * x * (3 - 2 * x);
}

float smootherstep(const float e0, const float e1, float x) {
    x = clamp(fast_divide((x - e0), (e1 - e0)), 0.0f, 1.0f);
    return x * x * x * (x * (x * 6 - 15) + 10);
}

uint32_t next_power_of_two(uint32_t x) {
    // FIXME: Protect against overflow
    uint32_t value = 1;

    while(value <= x) {
        value <<= 1;
    }

    return value;
}

__attribute__((optimize("O3", "fast-math")))
float fast_divide(float d, float n) {
#ifdef __DREAMCAST__
    const float sgn = ((*(unsigned int*)&n)>>31)*2+1;
    return sgn * (1.0f / __builtin_sqrtf(n * n)) * d;
#else
    return d / n;
#endif
}

__attribute__((optimize("O3", "fast-math")))
float fast_sqrt(float n) {
    return __builtin_sqrtf(n);
}

__attribute__((optimize("O3", "fast-math")))
void fast_sincos(double v, double* s, double* c) {
#ifdef __DREAMCAST__
    __builtin_sincos(v, s, c);
#else
    *s = sin(v);
    *c = cos(v);
#endif
}

__attribute__((optimize("O3", "fast-math")))
void fast_sincos(float v, float* s, float* c) {
#ifdef __DREAMCAST__
    __builtin_sincosf(v, s, c);
#else
    *s = sinf(v);
    *c = cosf(v);
#endif
}

__attribute__((optimize("O3", "fast-math")))
float fast_abs(float x) {
#ifdef __DREAMCAST__
    return __builtin_fabsf(x);
#else
    return std::abs(x);
#endif
}

__attribute__((optimize("O3", "fast-math")))
float fast_fmaf(float a, float b, float c) {
#ifdef __DREAMCAST__
    return __builtin_fmaf(a, b, c);
#else
    return fmaf(a, b, c);
#endif
}

__attribute__((optimize("O3", "fast-math")))
float fast_min(float a, float b) {
#ifdef __DREAMCAST__
    return __builtin_fminf(a, b);
#else
    return std::min(a, b);
#endif
}

__attribute__((optimize("O3", "fast-math")))
float fast_max(float a, float b) {
#ifdef __DREAMCAST__
    return __builtin_fmaxf(a, b);
#else
    return std::max(a, b);
#endif
}

__attribute__((optimize("O3", "fast-math")))
float fast_inverse_sqrt(float n) {
    return 1.0f / __builtin_sqrtf(n);
}

}
