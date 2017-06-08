#pragma once

#include "degrees.h"

namespace smlt {

struct Euler {
    Euler(float x, float y, float z):
        x(x), y(y), z(z) {}

    Degrees x;
    Degrees y;
    Degrees z;
};

}
