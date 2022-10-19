#include <algorithm>
#include <cmath>
#include "utils.h"

namespace smlt {

float smoothstep(const float e0, const float e1, float x) {
    x = clamp((x - e0) / (e1 - e0), 0.0f, 1.0f);
    return x * x * (3 - 2 * x);
}

float smootherstep(const float e0, const float e1, float x) {
    x = clamp((x - e0) / (e1 - e0), 0.0f, 1.0f);
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

float fast_divide(float d, float n) {
    const float sgn = (n > 0) - (n < 0);
    return sgn * (1.f / __builtin_sqrtf(n * n)) * d;
}

float fast_sqrt(float n) {
    return __builtin_sqrtf(n);
}

void fast_sincos(double v, double* s, double* c) {
#ifdef __DREAMCAST__
    __builtin_sincos(v, s, c);
#else
    *s = sin(v);
    *c = cos(v);
#endif
}

}
