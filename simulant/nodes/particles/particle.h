#pragma once

#include "../../math/vec3.h"
#include "../../math/vec2.h"
#include "../../colour.h"

namespace smlt {
namespace particles {

struct Particle {
    smlt::Vec3 position;
    smlt::Vec3 velocity;
    smlt::Vec2 dimensions;
    float ttl;
    smlt::Colour colour;
};

}
}
