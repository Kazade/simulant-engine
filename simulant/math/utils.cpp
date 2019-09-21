#include <algorithm>
#include "utils.h"

namespace smlt {

float clamp(float x, float l, float h) {
    return std::min(std::max(x, l), h);
}

float lerp(float x, float y, float t) {
    return x + (t * (y - x));
}

float smoothstep(float e0, float e1, float x) {
    x = clamp((x - e0) / (e1 - e0), 0.0f, 1.0f);
    return x * x * (3 - 2 * x);
}

float smootherstep(float e0, float e1, float x) {
    x = clamp((x - e0) / (e1 - e0), 0.0f, 1.0f);
    return x * x * x * (x * (x * 6 - 15) + 10);
}

}
