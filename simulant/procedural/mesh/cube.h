#ifndef CUBE_H
#define CUBE_H

#include "../constants.h"

#include "../../mesh.h"

namespace smlt {
namespace procedural {
namespace mesh {

void cube(MeshPtr mesh, float width, MeshStyle style=MESH_STYLE_SINGLE_SUBMESH);
void box(MeshPtr mesh, float width, float height, float depth, MeshStyle style=MESH_STYLE_SINGLE_SUBMESH);

}
}
}

#endif // CUBE_H
