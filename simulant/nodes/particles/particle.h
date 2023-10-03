#pragma once

#include "../../math/vec3.h"
#include "../../math/vec2.h"
#include "../../color.h"

namespace smlt {

struct Particle {
    smlt::Vec3 position;
    smlt::Vec3 velocity;
    smlt::Vec2 dimensions;
    smlt::Vec2 initial_dimensions;
    float ttl;
    float lifetime;
    smlt::Color color;
};

}
