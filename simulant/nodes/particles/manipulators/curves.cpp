#include <cmath>
#include <iostream>

float bell_curve(float initial, float t, float s, float peak, float deviation) {
    const int x = (int) (t * 100.0f);
    const float a = peak;
    const int b = (int) (0.5 * 100.0f);
    const int c = (int) (deviation * 100.0f);

    const float v = a * (float(exp(-(x - b) ^ 2 / (2 * c ^ 2))) / 100.0f);

    return initial + v;
}

float linear_curve(float initial, float t, float s, float rate) {
    return initial + (s * rate);
}
