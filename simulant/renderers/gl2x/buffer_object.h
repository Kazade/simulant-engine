#ifndef BUFFER_OBJECT_H
#define BUFFER_OBJECT_H

#include <cstdint>
#include <vector>

#include "../../generic/managed.h"
#include "glad/glad/glad.h"

namespace smlt {

enum BufferObjectType {
    BUFFER_OBJECT_VERTEX_DATA,
    BUFFER_OBJECT_INDEX_DATA
};

enum BufferObjectUsage {
    MODIFY_ONCE_USED_FOR_LIMITED_RENDERING,
    MODIFY_ONCE_USED_FOR_LIMITED_QUERYING,
    MODIFY_ONCE_USED_FOR_LIMITED_QUERYING_AND_RENDERING,
    MODIFY_ONCE_USED_FOR_RENDERING,
    MODIFY_ONCE_USED_FOR_QUERYING,
    MODIFY_ONCE_USED_FOR_QUERYING_AND_RENDERING,
    MODIFY_REPEATEDLY_USED_FOR_RENDERING,
    MODIFY_REPEATEDLY_USED_FOR_QUERYING,
    MODIFY_REPEATEDLY_USED_FOR_QUERYING_AND_RENDERING
};

class BufferObject : public Managed<BufferObject> {
public:
    BufferObject(BufferObjectType type, BufferObjectUsage usage=MODIFY_ONCE_USED_FOR_RENDERING);
    ~BufferObject();

    void bind();
    void build(uint32_t byte_size, const void* data);
    void modify(uint32_t offset, uint32_t byte_size, const void* data);
    void release();

    const std::vector<uint8_t>& offline_data() { return offline_data_; }

    GLenum usage() const;
    GLuint target() const { return gl_target_; }
private:

    BufferObjectUsage usage_;

    uint32_t gl_target_;
    uint32_t buffer_id_;

    std::vector<uint8_t> offline_data_;
};

class VertexArrayObject : public Managed<VertexArrayObject> {
public:
    VertexArrayObject(BufferObjectUsage vertex_usage=MODIFY_ONCE_USED_FOR_RENDERING, BufferObjectUsage index_usage=MODIFY_ONCE_USED_FOR_RENDERING);
    VertexArrayObject(BufferObject::ptr vertex_buffer, BufferObjectUsage index_usage=MODIFY_ONCE_USED_FOR_RENDERING);

    ~VertexArrayObject();

    void bind();

    void vertex_buffer_update(uint32_t byte_size, const void* data);
    void vertex_buffer_update_partial(uint32_t offset, uint32_t byte_size, const void* data);

    void index_buffer_update(uint32_t byte_size, const void* data);
    void index_buffer_update_partial(uint32_t offset, uint32_t byte_size, const void* data);

private:
    void vertex_buffer_bind() { vertex_buffer_->bind(); }
    void index_buffer_bind() { index_buffer_->bind(); }

    BufferObject::ptr vertex_buffer_;
    BufferObject::ptr index_buffer_;
};

}

#endif // BUFFER_OBJECT_H
