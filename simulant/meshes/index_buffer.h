#pragma once

#include <memory>

namespace smlt {

class VertexData;

/* Private renderer-specific data base class. This is used to
 * store things like VBO ids (in the case of OpenGL 2+) */
struct IndexBufferRendererData {
    virtual ~IndexBufferRendererData() {}
};

enum IndexFormat {
    INDEX_FORMAT_8BIT,
    INDEX_FORMAT_16BIT,
    INDEX_FORMAT_32BIT
};

class IndexBuffer {
private:
    friend class Renderer;

    IndexBuffer(IndexFormat format,
                std::shared_ptr<IndexBufferRendererData> renderer_data) :
        format_(format), renderer_data_(renderer_data) {}

public:
    IndexFormat format() const {
        return format_;
    }

    const std::shared_ptr<IndexBufferRendererData> renderer_data() const {
        return renderer_data_;
    }

private:
    IndexFormat format_;
    std::shared_ptr<IndexBufferRendererData> renderer_data_;
};

} // namespace smlt
