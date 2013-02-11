#ifndef SPHERE_H
#define SPHERE_H

#include <cstdint>

namespace kglt {

class Mesh;

namespace procedural {
namespace mesh {

void sphere(kglt::Mesh& mesh, float diameter, int32_t slices=20, int32_t stacks=20);
void icosphere(kglt::Mesh& mesh, float diameter);

}
}
}

#endif // SPHERE_H
