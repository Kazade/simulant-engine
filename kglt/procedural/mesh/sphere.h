#ifndef SPHERE_H
#define SPHERE_H

#include <cstdint>
#include "../../generic/protected_ptr.h"

namespace kglt {

class Mesh;

namespace procedural {
namespace mesh {

void sphere(ProtectedPtr<Mesh> mesh, float diameter, int32_t slices=20, int32_t stacks=20);
void icosphere(ProtectedPtr<Mesh> mesh, float diameter);

}
}
}

#endif // SPHERE_H
