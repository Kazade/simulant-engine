#pragma once

#include <cstdint>
#include "../../assets/material.h"

namespace smlt {
namespace utils {

struct Triangle {
    uint32_t idx[3];
    MaterialID mat;
};

void triangulate(MeshPtr mesh, std::vector<Vec3>& vertices, std::vector<Triangle>& triangles);

}
}
