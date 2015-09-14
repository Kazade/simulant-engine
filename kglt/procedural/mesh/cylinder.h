#ifndef CYLINDER_H
#define CYLINDER_H

#include <cstdint>
#include "../../generic/protected_ptr.h"

namespace kglt {

class Mesh;

namespace procedural {
namespace mesh {

void cylinder(ProtectedPtr<Mesh> mesh, float diameter, float length, int32_t segments=20, int32_t stacks=20);


}
}
}

#endif // CYLINDER_H

