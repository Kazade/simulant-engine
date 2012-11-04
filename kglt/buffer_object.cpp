#include "glee/GLee.h"
#include "kazbase/logging/logging.h"
#include "buffer_object.h"

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

    glGenBuffers(1, &buffer_id_);
    assert(buffer_id_ != 0);
}

BufferObject::~BufferObject() {

}

void BufferObject::bind() {
    glBindBuffer(gl_target_, buffer_id_);
}

void BufferObject::create(uint32_t byte_size, const void* data) {
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

    glBufferData(gl_target_, byte_size, data, usage);
}

void BufferObject::modify(uint32_t offset, uint32_t byte_size, const void* data) {
    glBufferSubData(gl_target_, offset, byte_size, data);
}

}
