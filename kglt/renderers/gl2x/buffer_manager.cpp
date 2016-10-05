#include <cassert>
#include "buffer_manager.h"
#include "../../utils/gl_error.h"

namespace kglt {

static GLenum convert_purpose(HardwareBufferPurpose purpose) {
    switch(purpose) {
    case HARDWARE_BUFFER_VERTEX_ATTRIBUTES: return GL_ARRAY_BUFFER;
    case HARDWARE_BUFFER_VERTEX_ARRAY_INDICES: return GL_ELEMENT_ARRAY_BUFFER;
    default:
        assert(0 && "Unsupported purpose");
    }
}

static GLenum convert_usage(HardwareBufferUsage usage) {
    switch(usage) {
        case HARDWARE_BUFFER_MODIFY_ONCE_USED_FOR_LIMITED_RENDERING:
            return GL_STREAM_DRAW;
        break;
        case HARDWARE_BUFFER_MODIFY_ONCE_USED_FOR_LIMITED_QUERYING:
#ifndef __ANDROID__
            return GL_STREAM_READ;
#else
            return GL_STREAM_DRAW;
#endif
        break;
        case HARDWARE_BUFFER_MODIFY_ONCE_USED_FOR_LIMITED_QUERYING_AND_RENDERING:
#ifndef __ANDROID__
            return GL_STREAM_COPY;
#else
            return GL_STREAM_DRAW;
#endif
        break;
        case HARDWARE_BUFFER_MODIFY_ONCE_USED_FOR_RENDERING:
            return GL_STATIC_DRAW;
        break;
        case HARDWARE_BUFFER_MODIFY_ONCE_USED_FOR_QUERYING:
#ifndef __ANDROID__
            return GL_STATIC_READ;
#else
            return GL_STATIC_DRAW;
#endif
        break;
        case HARDWARE_BUFFER_MODIFY_ONCE_USED_FOR_QUERYING_AND_RENDERING:
#ifndef __ANDROID__
            return GL_STATIC_COPY;
#else
            return GL_STATIC_DRAW;
#endif
        break;
        case HARDWARE_BUFFER_MODIFY_REPEATEDLY_USED_FOR_RENDERING:
            return GL_DYNAMIC_DRAW;
        break;
        case HARDWARE_BUFFER_MODIFY_REPEATEDLY_USED_FOR_QUERYING:
#ifndef __ANDROID__
            return GL_DYNAMIC_READ;
#else
            return GL_DYNAMIC_DRAW;
#endif
        break;
        case HARDWARE_BUFFER_MODIFY_REPEATEDLY_USED_FOR_QUERYING_AND_RENDERING:
#ifndef __ANDROID__
            return GL_DYNAMIC_COPY;
#else
            return GL_DYNAMIC_DRAW;
#endif
        break;
        default:
            assert(0 && "Unsupported usage");
    }
}

std::unique_ptr<HardwareBufferImpl> GL2BufferManager::do_allocation(std::size_t size, HardwareBufferPurpose purpose, HardwareBufferUsage usage) {
    std::unique_ptr<GL2HardwareBufferImpl> buffer_impl(new GL2HardwareBufferImpl(this));

    buffer_impl->size = size;
    buffer_impl->offset = 0;
    buffer_impl->usage = convert_usage(usage);
    buffer_impl->purpose = convert_purpose(purpose);

    GLCheck(glGenBuffers, 1, &buffer_impl->buffer_id);
    GLCheck(glBindBuffer, buffer_impl->purpose, buffer_impl->buffer_id);
    GLCheck(glBufferData, buffer_impl->purpose, size, nullptr, buffer_impl->usage);

    return std::move(buffer_impl);
}

void GL2BufferManager::do_release(const HardwareBufferImpl *buffer) {
    auto gl2_buffer = static_cast<const GL2HardwareBufferImpl*>(buffer);
    GLCheck(glDeleteBuffers, 1, &gl2_buffer->buffer_id);
}

void GL2HardwareBufferImpl::upload(const uint8_t *data, const std::size_t size) {
    GLCheck(glBindBuffer, purpose, buffer_id);
    GLCheck(glBufferSubData, purpose, offset, size, data);
}

}
