#include <GLee.h>
#include "kazbase/logging.h"
#include "buffer_object.h"
#include "utils/gl_thread_check.h"

namespace kglt {

BufferObject::BufferObject(BufferObjectType type, BufferObjectUsage usage):
    usage_(usage),
    gl_target_(0),
    buffer_id_(0),
    initialized_(false) {

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

}

void BufferObject::bind() const {
    GLThreadCheck::check();

    assert(initialized_);
    assert(buffer_id_);
    glBindBuffer(gl_target_, buffer_id_);
}

void BufferObject::create(uint32_t byte_size, const void* data) {
    GLThreadCheck::check();

    if(!buffer_id_) {
        glGenBuffers(1, &buffer_id_);
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

    glBindBuffer(gl_target_, buffer_id_);
    assert(glGetError() == 0);
    glBufferData(gl_target_, byte_size, data, usage);
    assert(glGetError() == 0);
    initialized_ = true;
}

void BufferObject::modify(uint32_t offset, uint32_t byte_size, const void* data) {
    GLThreadCheck::check();

    assert(buffer_id_);

    glBindBuffer(gl_target_, buffer_id_);
    glBufferSubData(gl_target_, offset, byte_size, data);
}

}
