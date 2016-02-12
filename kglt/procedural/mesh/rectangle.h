#ifndef RECTANGLE_H_INCLUDED
#define RECTANGLE_H_INCLUDED

#include "kglt/mesh.h"
#include "../../generic/protected_ptr.h"

namespace kglt {

class Mesh;

namespace procedural {
namespace mesh {

SubMeshID new_rectangle_submesh(
    ProtectedPtr<Mesh>& mesh,
    float width,
    float height,
    float x_offset=0.0, float y_offset=0.0, float z_offset=0.0,
    MaterialID material_id=MaterialID()
);

SubMeshID rectangle(ProtectedPtr<Mesh> mesh,
    float width, float height,
    float x_offset=0.0, float y_offset=0.0, float z_offset=0.0,
    bool clear=true, kglt::MaterialID material=kglt::MaterialID());

SubMeshID rectangle_outline(
    ProtectedPtr<Mesh> mesh,
    float width, float height,
    float x_offset=0.0, float y_offset=0.0, float z_offset=0.0,
    bool clear=true, kglt::MaterialID material=kglt::MaterialID());

}
}
}


#endif // RECTANGLE_H_INCLUDED
