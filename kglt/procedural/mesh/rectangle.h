#ifndef RECTANGLE_H_INCLUDED
#define RECTANGLE_H_INCLUDED

#include "kglt/mesh.h"

namespace kglt {

class Mesh;

namespace procedural {
namespace mesh {

SubMesh* new_rectangle_submesh(
    MeshPtr& mesh,
    float width,
    float height,
    float x_offset=0.0, float y_offset=0.0, float z_offset=0.0,
    MaterialID material_id=MaterialID()
);

SubMesh* rectangle(MeshPtr mesh,
    float width, float height,
    float x_offset=0.0, float y_offset=0.0, float z_offset=0.0,
    bool clear=true, kglt::MaterialID material=kglt::MaterialID());

kglt::SubMesh *rectangle_outline(
    MeshPtr mesh,
    float width, float height,
    float x_offset=0.0, float y_offset=0.0, float z_offset=0.0,
    bool clear=true, kglt::MaterialID material=kglt::MaterialID());

}
}
}


#endif // RECTANGLE_H_INCLUDED
