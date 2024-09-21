#pragma once

#include "vertex_format.h"
#include <memory>

/* A VertexBuffer is renderer-specific storage for vertex data.
 *
 * VertexBuffers have a VertexFormat, just like mesh vertex data
 * but the format is one that is compatible with the current renderer.
 *
 * Vertex buffers are created by the Renderer, using VertexData as a source.
 *
 * A renderable can either contain VertexData* or a VertexBuffer*, if it returns
 * VertexData* then the render will compile that into a dynamic VertexBuffer*
 * but this will not be performant.
 *
 * Meshes will have a VertexBuffer if their data has been "flushed" to the GPU.
 * Optionally Meshes that have been flushed by lose their VertexData* to free up
 * RAM.
 */

namespace smlt {

class VertexData;

/* Private renderer-specific data base class. This is used to
 * store things like VBO ids (in the case of OpenGL 2+) or
 * converted vertex data (in the case of OpenGL 1.x) */
struct VertexBufferRendererData {
    virtual ~VertexBufferRendererData() {}
};

class VertexBuffer {
private:
    friend class Renderer;

    VertexBuffer(VertexFormat format,
                 std::shared_ptr<VertexBufferRendererData> renderer_data) :
        format_(format), renderer_data_(renderer_data) {}

public:
    VertexFormat format() const {
        return format_;
    }

    const std::shared_ptr<VertexBufferRendererData> renderer_data() const {
        return renderer_data_;
    }

private:
    VertexFormat format_;
    std::shared_ptr<VertexBufferRendererData> renderer_data_;
};

} // namespace smlt
