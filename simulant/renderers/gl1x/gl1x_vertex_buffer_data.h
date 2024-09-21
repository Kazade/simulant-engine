#pragma once

#include "../../core/aligned_vector.h"
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
    aligned_vector<GL1Vertex, 32> vertices;
};

} // namespace smlt
