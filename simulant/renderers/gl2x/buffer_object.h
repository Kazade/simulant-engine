/* *   Copyright (c) 2011-2017 Luke Benstead https://simulant-engine.appspot.com
 *
 *     This file is part of Simulant.
 *
 *     Simulant is free software: you can redistribute it and/or modify
 *     it under the terms of the GNU Lesser General Public License as published by
 *     the Free Software Foundation, either version 3 of the License, or
 *     (at your option) any later version.
 *
 *     Simulant is distributed in the hope that it will be useful,
 *     but WITHOUT ANY WARRANTY; without even the implied warranty of
 *     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *     GNU Lesser General Public License for more details.
 *
 *     You should have received a copy of the GNU Lesser General Public License
 *     along with Simulant.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef BUFFER_OBJECT_H
#define BUFFER_OBJECT_H

#include <cstdint>
#include <vector>

#include "../../generic/managed.h"

#ifdef __ANDROID__
    #include <GLES2/gl2.h>
    #include <GLES2/gl2ext.h>
#else
    #include "../glad/glad/glad.h"
#endif

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
