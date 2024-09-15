#pragma once

#include "../../meshes/vertex_buffer.h"
#include "vbo_manager.h"

namespace smlt {

struct GL2VertexBufferRendererData: public VertexBufferRendererData {
    GPUBuffer vertex_buffer;
    GPUBuffer index_buffer;
};

} // namespace smlt
