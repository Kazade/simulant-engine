#include <algorithm>
#include <cmath>
#include "utils.h"

#ifdef __DREAMCAST__
#include "../utils/sh4_math.h"
#endif

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

#ifdef __DREAMCAST__
static inline __attribute__((always_inline)) float fsrra(float x) {
    asm volatile ("fsrra %[one_div_sqrt]\n"
    : [one_div_sqrt] "+f" (x) // outputs, "+" means r/w
    : // no inputs
    : // no clobbers
    );

    return x;
}
#endif

float fast_divide(float d, float n) {
#ifdef __DREAMCAST__
    const float sgn = (n > 0) - (n < 0);
    return sgn * fsrra(n * n) * d;
#else
    return d / n;
#endif
}

}
