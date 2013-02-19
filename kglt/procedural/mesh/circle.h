#ifndef CIRCLE_H
#define CIRCLE_H

#include "kglt/mesh.h"

namespace kglt {
namespace procedural {
namespace mesh {

void circle(
    kglt::Mesh& mesh,
    float diameter,
    float x_offset=0.0, float y_offset=0.0, float z_offset=0.0,
    bool clear=true
);

void circle_outline(
    kglt::Mesh& mesh,
    float diameter,
    float x_offset=0.0, float y_offset=0.0, float z_offset=0.0,
    bool clear=true
);

}
}
}

#endif // CIRCLE_H
