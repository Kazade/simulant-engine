#include <cmath>
#include <iostream>

float bell_curve(float initial, float t, float s, float peak, float deviation) {
    const int x = (int) (t * 100.0f);
    const float a = peak;
    const int b = 50;
    const int c = (int) (deviation * 100.0f);

    const float y = a * exp(-(pow(x - b, 2) / pow(2 * c, 2)));

    return initial + y;
}

float linear_curve(float initial, float t, float s, float rate) {
    return initial + (s * rate);
}
