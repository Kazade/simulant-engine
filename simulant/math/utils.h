#pragma once

#include <cmath>

namespace smlt {

const float PI = acos(-1);
const float TWO_PI = PI * 2.0f;
const float PI_OVER_180 = PI / 180.0f;
const float PI_UNDER_180 = 180.0f / PI;

float clamp(float x, float l, float h);
float lerp(float x, float y, float t);

}
