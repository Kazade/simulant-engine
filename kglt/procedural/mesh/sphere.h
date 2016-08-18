#ifndef SPHERE_H
#define SPHERE_H

#include <cstdint>
#include "../../types.h"

namespace kglt {

class Mesh;

namespace procedural {
namespace mesh {

void sphere(MeshPtr mesh, float diameter, int32_t slices=20, int32_t stacks=20);
void icosphere(MeshPtr mesh, float diameter);

}
}
}

#endif // SPHERE_H
