/* Only included from mesh.cpp and submesh.cpp */

#pragma once

#include "../hardware_buffer.h"
#include "../renderers/renderer.h"

namespace smlt {

template<typename Data, typename Allocator>
void sync_buffer(HardwareBuffer::ptr* buffer, Data* data, Allocator* allocator, HardwareBufferPurpose purpose) {
    if(!(*buffer) && data->count()) {
        (*buffer) = allocator->hardware_buffers->allocate(
            data->data_size(),
            purpose,
            SHADOW_BUFFER_DISABLED
        );
    } else {
        assert(data->count());
        (*buffer)->resize(data->data_size());
    }

    (*buffer)->upload(*data);
}

}
