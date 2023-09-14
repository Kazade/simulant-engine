#include <cmath>
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



}
