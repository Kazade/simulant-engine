#include <cassert>

#include "utils/glcompat.h"

#include "kazbase/logging.h"
#include "buffer_object.h"
#include "utils/gl_thread_check.h"
#include "utils/gl_error.h"

#include "utils/vao_abstraction.h"

namespace kglt {

bool VertexArrayObject::VAO_SUPPORTED = false;

BufferObject::BufferObject(BufferObjectType type, BufferObjectUsage usage):
    usage_(usage),
    gl_target_(0),
    buffer_id_(0) {

    //FIXME: Totally need to support more than this
    switch(type) {
        case BUFFER_OBJECT_VERTEX_DATA:
            gl_target_ = GL_ARRAY_BUFFER;
        break;
        case BUFFER_OBJECT_INDEX_DATA:
            gl_target_ = GL_ELEMENT_ARRAY_BUFFER;
        break;
        default:
            L_WARN("We don't yet support this shizzle");
    }    
}

BufferObject::~BufferObject() {
    try {
        release();
    } catch(...) {
        return;
    }
}

void BufferObject::release() {
    if(buffer_id_) {
        GLCheck(glDeleteBuffers, 1, &buffer_id_);
    }
}

void BufferObject::bind() {
    if(!buffer_id_) {
        GLCheck(glGenBuffers, 1, &buffer_id_);
    }

    GLCheck(glBindBuffer, gl_target_, buffer_id_);
}

GLenum BufferObject::usage() const {
    GLenum usage;
    switch(usage_) {
        case MODIFY_ONCE_USED_FOR_LIMITED_RENDERING:
            usage = GL_STREAM_DRAW;
        break;
        case MODIFY_ONCE_USED_FOR_LIMITED_QUERYING:
#ifndef __ANDROID__
            usage = GL_STREAM_READ;
#else
            usage = GL_STREAM_DRAW;
#endif
        break;
        case MODIFY_ONCE_USED_FOR_LIMITED_QUERYING_AND_RENDERING:
#ifndef __ANDROID__
            usage = GL_STREAM_COPY;
#else
            usage = GL_STREAM_DRAW;
#endif
        break;
        case MODIFY_ONCE_USED_FOR_RENDERING:
            usage = GL_STATIC_DRAW;
        break;
        case MODIFY_ONCE_USED_FOR_QUERYING:
#ifndef __ANDROID__
            usage = GL_STATIC_READ;
#else
            usage = GL_STATIC_DRAW;
#endif
        break;
        case MODIFY_ONCE_USED_FOR_QUERYING_AND_RENDERING:
#ifndef __ANDROID__
            usage = GL_STATIC_COPY;
#else
            usage = GL_STATIC_DRAW;
#endif
        break;
        case MODIFY_REPEATEDLY_USED_FOR_RENDERING:
            usage = GL_DYNAMIC_DRAW;
        break;
        case MODIFY_REPEATEDLY_USED_FOR_QUERYING:
#ifndef __ANDROID__
            usage = GL_DYNAMIC_READ;
#else
            usage = GL_DYNAMIC_DRAW;
#endif
        break;
        case MODIFY_REPEATEDLY_USED_FOR_QUERYING_AND_RENDERING:
#ifndef __ANDROID__
            usage = GL_DYNAMIC_COPY;
#else
            usage = GL_DYNAMIC_DRAW;
#endif
        break;
        default:
            throw std::logic_error("What the...?");
    }

    return usage;
}

void BufferObject::create(uint32_t byte_size, const void* data) {
    if(!buffer_id_) {
        GLCheck(glGenBuffers, 1, &buffer_id_);
    }

    assert(buffer_id_);

    GLCheck(glBindBuffer, gl_target_, buffer_id_);
    GLCheck(glBufferData, gl_target_, byte_size, data, usage());
}

void BufferObject::modify(uint32_t offset, uint32_t byte_size, const void* data) {
    assert(buffer_id_);

    GLCheck(glBindBuffer, gl_target_, buffer_id_);
    GLCheck(glBufferSubData, gl_target_, offset, byte_size, data);
}

VertexArrayObject::VertexArrayObject(BufferObjectUsage vertex_usage, BufferObjectUsage index_usage):
    vertex_buffer_(BUFFER_OBJECT_VERTEX_DATA, vertex_usage),
    index_buffer_(BUFFER_OBJECT_INDEX_DATA, index_usage),
    id_(0) {
}

VertexArrayObject::~VertexArrayObject() {
    try {
        if(id_) {
            GLCheck(vaoDeleteVertexArrays, 1, &id_);
        }
    } catch(...) {}
}

void VertexArrayObject::bind() {
    if(id_ == 0) {
        GLCheck(vaoGenVertexArrays, 1, &id_);
        assert(id_);
    }
    GLCheck(vaoBindVertexArray, id_);

}

void VertexArrayObject::vertex_buffer_update(uint32_t byte_size, const void* data) {
    GLStateStash stash(GL_VERTEX_ARRAY_BINDING); //Store the current VAO binding

    bind();

    vertex_buffer_.create(byte_size, data);
}

void VertexArrayObject::vertex_buffer_update_partial(uint32_t offset, uint32_t byte_size, const void* data) {
    GLStateStash stash(GL_VERTEX_ARRAY_BINDING); //Store the current VAO binding

    bind();

    vertex_buffer_.modify(offset, byte_size, data);
}

void VertexArrayObject::index_buffer_update(uint32_t byte_size, const void* data) {
    GLStateStash stash(GL_VERTEX_ARRAY_BINDING); //Store the current VAO binding

    bind();

    index_buffer_.create(byte_size, data);
}

void VertexArrayObject::index_buffer_update_partial(uint32_t offset, uint32_t byte_size, const void* data) {
    GLStateStash stash(GL_VERTEX_ARRAY_BINDING); //Store the current VAO binding

    bind();

    index_buffer_.modify(offset, byte_size, data);
}

}
