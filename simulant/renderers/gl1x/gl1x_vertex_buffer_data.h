#pragma once

#include "../../meshes/vertex_buffer.h"
#include <vector>

namespace smlt {

struct alignas(32) GL1Vertex {
    Vec3 xyz;
    Vec2 uv;
    uint32_t color;
    Vec3 n;
};

struct GL1XVertexBufferData: public VertexBufferRendererData {
    std::vector<GL1Vertex> vertices;
};

} // namespace smlt
