#ifndef CIRCLE_H
#define CIRCLE_H

#include "kglt/mesh.h"

namespace kglt {
namespace procedural {
namespace mesh {

SubMeshID circle(
    kglt::Mesh& mesh,
    float diameter=1.0,
    int32_t point_count=20,
    float x_offset=0.0, float y_offset=0.0, float z_offset=0.0
);

SubMeshID circle_outline(
    kglt::Mesh& mesh,
    float diameter=1.0,
    int32_t point_count=20,
    float x_offset=0.0, float y_offset=0.0, float z_offset=0.0
);

}
}
}

#endif // CIRCLE_H
