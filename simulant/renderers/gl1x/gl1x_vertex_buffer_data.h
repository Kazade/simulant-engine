#pragma once

#include "../../core/aligned_vector.h"
#include "../../meshes/vertex_buffer.h"
#include <vector>

namespace smlt {

struct alignas(32) GL1Vertex {
    Vec3 xyz;    // 12
    Vec2 uv;     // 8
    Color color; // 16

    Vec3 n; // 12
    Vec3 t; // 12 Tangent
    Vec3 b; // 12 Bitangent
};

struct GL1XVertexBufferData: public VertexBufferRendererData {
    std::vector<GL1Vertex> vertices;

    /* This is transient data calculated for each renderable
     * so we can do lighting within the engine instead of
     * using GL1 lighting */
    std::vector<Vec3> eye_space_positions;
    std::vector<Vec3> eye_space_normals;
    std::vector<Vec3> tangent_space_light_dirs;
    std::vector<uint32_t> colors;

    static const VertexFormat format;
};

} // namespace smlt
