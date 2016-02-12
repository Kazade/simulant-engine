#ifndef CUBE_H
#define CUBE_H

#include "../constants.h"

#include "kglt/mesh.h"

namespace kglt {
namespace procedural {
namespace mesh {

void cube(ProtectedPtr<Mesh> mesh, float width, MeshStyle style=MESH_STYLE_SINGLE_SUBMESH);
void box(ProtectedPtr<Mesh> mesh, float width, float height, float depth, MeshStyle style=MESH_STYLE_SINGLE_SUBMESH);

}
}
}

#endif // CUBE_H
