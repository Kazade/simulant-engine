#ifndef RECTANGLE_H_INCLUDED
#define RECTANGLE_H_INCLUDED

#include "kglt/mesh.h"

namespace kglt {
namespace procedural {
namespace mesh {

SubMeshIndex rectangle(
    kglt::MeshPtr mesh,
    float width, float height,
    float x_offset=0.0, float y_offset=0.0, float z_offset=0.0,
    bool clear=true
);

SubMeshIndex rectangle_outline(
    kglt::MeshPtr mesh,
    float width, float height,
    float x_offset=0.0, float y_offset=0.0, float z_offset=0.0,
    bool clear=true
);

}
}
}


#endif // RECTANGLE_H_INCLUDED
