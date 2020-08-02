#include <algorithm>
#include "utils.h"

namespace smlt {

float clamp(const float x, const float l, const float h) {
    return std::min(std::max(x, l), h);
}

float lerp(const float x, const float y, const float t) {
    return x + (t * (y - x));
}

float smoothstep(const float e0, const float e1, float x) {
    x = clamp((x - e0) / (e1 - e0), 0.0f, 1.0f);
    return x * x * (3 - 2 * x);
}

float smootherstep(const float e0, const float e1, float x) {
    x = clamp((x - e0) / (e1 - e0), 0.0f, 1.0f);
    return x * x * x * (x * (x * 6 - 15) + 10);
}

}
