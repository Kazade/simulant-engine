#pragma once

#include <cstdint>

namespace smlt {

enum GeomCullerType {
    GEOM_CULLER_TYPE_OCTREE,
    GEOM_CULLER_TYPE_QUADTREE
};

struct GeomCullerOptions {
    GeomCullerType type = GEOM_CULLER_TYPE_OCTREE;
    uint8_t octree_max_depth = 4;
    uint8_t quadtree_max_depth = 4;
};

} // namespace smlt
