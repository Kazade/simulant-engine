#ifndef CAPSULE_H
#define CAPSULE_H

#include "../../mesh.h"

namespace smlt {
namespace procedural {
namespace mesh {

smlt::SubMesh* capsule(
    MeshPtr mesh,
    float diameter=0.5,
    float height=1.0,
    uint32_t segment_count=10,
    uint32_t vertical_segment_count=1,
    uint32_t ring_count=10,
    const smlt::Vec3& pos_offset=smlt::Vec3()
);

}
}
}

#endif // CAPSULE_H
