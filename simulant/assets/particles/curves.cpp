#include <cmath>
#include <iostream>

#include "../../macros.h"

float bell_curve(float initial, float t, float s, float peak, float deviation) {
    _S_UNUSED(s);

    const int x = (int) (t * 100.0f);
    const float a = peak;
    const int b = 50;
    const int c = (int) (deviation * 100.0f);

    const float y = a * expf(-(powf(x - b, 2) / powf(2 * c, 2)));

    return initial + y;
}

float linear_curve(float initial, float t, float s, float rate) {
    _S_UNUSED(t);

    return initial + (s * rate);
}
