#ifndef CUBE_H
#define CUBE_H

#include "kglt/mesh.h"

namespace kglt {
namespace procedural {
namespace mesh {

void cube(ProtectedPtr<Mesh> mesh, float width);
void box(ProtectedPtr<Mesh> mesh, float width, float height, float depth);

}
}
}

#endif // CUBE_H
