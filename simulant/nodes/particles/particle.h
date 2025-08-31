#pragma once

#include "../../color.h"
#include "../../math/vec2.h"
#include "../../math/vec3.h"
#include <cstdint>

namespace smlt {

struct Particle {
    smlt::Vec3 position;
    smlt::Vec3 velocity;
    smlt::Vec2 dimensions;
    smlt::Vec2 initial_dimensions;
    float ttl;
    float lifetime;
    smlt::Color color;

    /* We need to store this for local space
     * particle systems as we need to look up the
     * position of the emitter */
    uint8_t emitter_index;
};

}
