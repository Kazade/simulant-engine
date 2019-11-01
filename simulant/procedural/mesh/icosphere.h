#pragma once

#include <cstdint>

namespace smlt {

class SubMesh;

namespace procedural {
namespace mesh {

void icosphere(SubMesh* out, float diameter, uint32_t subdivisions);

}
}
}
