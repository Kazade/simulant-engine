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

    /* This is the color submitted by the user, the color above
     * is the final calculated color after lighting has taken place.
     *
     * This is stored as a float to make the lighting calculations easier
     * and we convert to packed argb at the end
     */
    Color submitted_color;
};

struct GL1XVertexBufferData: public VertexBufferRendererData {
    std::vector<GL1Vertex> vertices;

    static const VertexFormat format;
};

} // namespace smlt
