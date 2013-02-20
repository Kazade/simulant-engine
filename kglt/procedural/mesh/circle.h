#ifndef CIRCLE_H
#define CIRCLE_H

#include "kglt/mesh.h"

namespace kglt {
namespace procedural {
namespace mesh {

SubMeshIndex circle(
    kglt::Mesh& mesh,
    float diameter,
    int32_t point_count,
    float x_offset=0.0, float y_offset=0.0, float z_offset=0.0
);

SubMeshIndex circle_outline(
    kglt::Mesh& mesh,
    float diameter,
    int32_t point_count,
    float x_offset=0.0, float y_offset=0.0, float z_offset=0.0
);

}
}
}

#endif // CIRCLE_H
