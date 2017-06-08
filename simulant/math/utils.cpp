#include <algorithm>
#include "utils.h"

namespace smlt {

float clamp(float x, float l, float h) {
    return std::min(std::max(x, l), h);
}

float lerp(float x, float y, float t) {
    return x + (t * (y - x));
}

}
