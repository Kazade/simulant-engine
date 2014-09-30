#include <cassert>

#include "utils/glcompat.h"

#include "kazbase/logging.h"
#include "buffer_object.h"
#include "utils/gl_thread_check.h"
#include "utils/gl_error.h"

#ifdef __ANDROID__

#include <EGL/egl.h>
#include <GLES2/gl2ext.h>

PFNGLGENVERTEXARRAYSOESPROC glGenVertexArrays = nullptr;
PFNGLBINDVERTEXARRAYOESPROC glBindVertexArray = nullptr;
PFNGLDELETEVERTEXARRAYSOESPROC glDeleteVertexArrays = nullptr;

#endif

namespace kglt {

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


#ifdef __ANDROID__
    if(!glGenVertexArrays) {
        glGenVertexArrays = (PFNGLGENVERTEXARRAYSOESPROC) eglGetProcAddress("glGenVertexArraysOES");
        glBindVertexArray = (PFNGLBINDVERTEXARRAYOESPROC) eglGetProcAddress("glBindVertexArrayOES");
        glDeleteVertexArrays = (PFNGLDELETEVERTEXARRAYSOESPROC) eglGetProcAddress("glDeleteVertexArraysOES");

        if(!(glGenVertexArrays && glBindVertexArray && glDeleteVertexArrays)) {
            throw RuntimeError("glGenVertexArraysOES is not supported on this device");
        }
    }
#endif
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
    GLThreadCheck::check();

    if(!buffer_id_) {
        GLCheck(glGenBuffers, 1, &buffer_id_);
    }

    GLCheck(glBindBuffer, gl_target_, buffer_id_);
}

void BufferObject::create(uint32_t byte_size, const void* data) {
    GLThreadCheck::check();

    if(!buffer_id_) {
        GLCheck(glGenBuffers, 1, &buffer_id_);
    }

    assert(buffer_id_);

    GLenum usage;
    switch(usage_) {
        case MODIFY_ONCE_USED_FOR_LIMITED_RENDERING:
            usage = GL_STREAM_DRAW;
        break;
        case MODIFY_ONCE_USED_FOR_LIMITED_QUERYING:
            usage = GL_STREAM_READ;
        break;
        case MODIFY_ONCE_USED_FOR_LIMITED_QUERYING_AND_RENDERING:
            usage = GL_STREAM_COPY;
        break;
        case MODIFY_ONCE_USED_FOR_RENDERING:
            usage = GL_STATIC_DRAW;
        break;
        case MODIFY_ONCE_USED_FOR_QUERYING:
            usage = GL_STATIC_READ;
        break;
        case MODIFY_ONCE_USED_FOR_QUERYING_AND_RENDERING:
            usage = GL_STATIC_COPY;
        break;
        case MODIFY_REPEATEDLY_USED_FOR_RENDERING:
            usage = GL_DYNAMIC_DRAW;
        break;
        case MODIFY_REPEATEDLY_USED_FOR_QUERYING:
            usage = GL_DYNAMIC_READ;
        break;
        case MODIFY_REPEATEDLY_USED_FOR_QUERYING_AND_RENDERING:
            usage = GL_DYNAMIC_COPY;
        break;
        default:
            throw std::logic_error("What the...?");
    }

    GLCheck(glBindBuffer, gl_target_, buffer_id_);
    GLCheck(glBufferData, gl_target_, byte_size, data, usage);
}

void BufferObject::modify(uint32_t offset, uint32_t byte_size, const void* data) {
    GLThreadCheck::check();

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
            GLCheck(glDeleteVertexArrays, 1, &id_);
        }
    } catch(...) {}
}

void VertexArrayObject::bind() {
    if(id_ == 0) {
        GLCheck(glGenVertexArrays, 1, &id_);
        assert(id_);
    }
    GLCheck(glBindVertexArray, id_);
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
