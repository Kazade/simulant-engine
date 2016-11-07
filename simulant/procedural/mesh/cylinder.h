#ifndef CYLINDER_H
#define CYLINDER_H

#include <cstdint>
#include "../../types.h"

namespace smlt {

class Mesh;

namespace procedural {
namespace mesh {

void cylinder(MeshPtr mesh, float diameter, float length, int32_t segments=20, int32_t stacks=20);


}
}
}

#endif // CYLINDER_H

