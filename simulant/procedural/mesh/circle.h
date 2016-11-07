#ifndef CIRCLE_H
#define CIRCLE_H

#include "../../mesh.h"

namespace smlt {
namespace procedural {
namespace mesh {

SubMesh* circle(
    smlt::Mesh& mesh,
    float diameter=1.0,
    int32_t point_count=20,
    float x_offset=0.0, float y_offset=0.0, float z_offset=0.0
);

SubMesh* circle_outline(
    smlt::Mesh& mesh,
    float diameter=1.0,
    int32_t point_count=20,
    float x_offset=0.0, float y_offset=0.0, float z_offset=0.0
);

}
}
}

#endif // CIRCLE_H
