#include <cassert>
#include "buffer_manager.h"
#include "../renderer.h"
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

GL2BufferManager::GL2BufferManager(const Renderer* renderer):
    HardwareBufferManager(renderer) {

}

std::unique_ptr<HardwareBufferImpl> GL2BufferManager::do_allocation(std::size_t size, HardwareBufferPurpose purpose, HardwareBufferUsage usage) {
    std::unique_ptr<GL2HardwareBufferImpl> buffer_impl(new GL2HardwareBufferImpl(this));

    buffer_impl->size = size;
    buffer_impl->capacity = size; // FIXME: Should probably round to some boundary
    buffer_impl->offset = 0;
    buffer_impl->usage = convert_usage(usage);
    buffer_impl->purpose = convert_purpose(purpose);

    auto allocate_buffers = [&buffer_impl]() {
        GLCheck(glGenBuffers, 1, &buffer_impl->buffer_id);
        GLCheck(glBindBuffer, buffer_impl->purpose, buffer_impl->buffer_id);
        GLCheck(glBufferData, buffer_impl->purpose, buffer_impl->capacity, nullptr, buffer_impl->usage);
    };

    if(GLThreadCheck::is_current()) {
        allocate_buffers();
    } else {
        auto& idle_manager = renderer->window->idle;
        idle_manager->add_once(allocate_buffers);
        idle_manager->wait();
    }

    return std::move(buffer_impl);
}

void GL2BufferManager::do_release(const HardwareBufferImpl *buffer) {
    auto gl2_buffer = static_cast<const GL2HardwareBufferImpl*>(buffer);
    if(GLThreadCheck::is_current()) {
        GLCheck(glDeleteBuffers, 1, &gl2_buffer->buffer_id);
    } else {
        auto& idle_manager = renderer->window->idle;
        // Make sure we run the GL stuff on the main thread
        idle_manager->add_once([&]() {
            GLCheck(glDeleteBuffers, 1, &gl2_buffer->buffer_id);
        });

        idle_manager->wait();
    }
}

void GL2BufferManager::do_resize(HardwareBufferImpl* buffer, std::size_t new_size) {
    auto gl2_buffer = static_cast<GL2HardwareBufferImpl*>(buffer);

    auto resize_buffer = [gl2_buffer, new_size]() {
        //FIXME: If supported this should use glCopyBufferSubData for performance
        GLCheck(glBindBuffer, gl2_buffer->purpose, gl2_buffer->buffer_id);

        // Read the data from the existing buffer so we can retain it
        std::vector<uint8_t> existing(gl2_buffer->size);
        GLCheck(glGetBufferSubData, gl2_buffer->purpose, gl2_buffer->offset, gl2_buffer->size, &existing[0]);

        // Resize to the new size, either truncating the data, or extending it with zeroes
        existing.resize(new_size, 0);

        // Update the size of the allocated buffer
        gl2_buffer->size = new_size;
        gl2_buffer->capacity = new_size; //FIXME: Should probably round up (like do_allocate)

        // Upload the new data to the existing buffer
        GLCheck(glBufferData, gl2_buffer->purpose, gl2_buffer->capacity, &existing[0], gl2_buffer->usage);
    };

    if(GLThreadCheck::is_current()) {
        resize_buffer();
    } else {
        auto& idle_manager = renderer->window->idle;
        idle_manager->add_once(resize_buffer);
        idle_manager->wait();
    }
}

void GL2BufferManager::do_bind(const HardwareBufferImpl *buffer, HardwareBufferPurpose purpose) {
    auto gl2_buffer = static_cast<const GL2HardwareBufferImpl*>(buffer);
    if(GLThreadCheck::is_current()) {
        GLCheck(glBindBuffer, convert_purpose(purpose), gl2_buffer->buffer_id);
    } else {
        auto& idle_manager = renderer->window->idle;
        // If we're uploading in a background thread, make sure we run the GL stuff on the main thread
        idle_manager->add_once([&]() {
            GLCheck(glBindBuffer, convert_purpose(purpose), gl2_buffer->buffer_id);
        });
        idle_manager->wait();
    }
}

void GL2HardwareBufferImpl::upload(const uint8_t *data, const std::size_t size) {
    assert(size <= capacity);

    if(GLThreadCheck::is_current()) {
        GLCheck(glBindBuffer, purpose, buffer_id);
        GLCheck(glBufferSubData, purpose, offset, size, data);
    } else {
        auto& idle_manager = manager->renderer->window->idle;
        // If we're uploading in a background thread, make sure we run the GL stuff on the main thread
        idle_manager->add_once([&]() {
            GLCheck(glBindBuffer, purpose, buffer_id);
            GLCheck(glBufferSubData, purpose, offset, size, data);
        });
        idle_manager->wait();
    }
}

}
