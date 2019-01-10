/* Only included from mesh.cpp and submesh.cpp */

#pragma once

#include "../hardware_buffer.h"
#include "../renderers/renderer.h"

namespace smlt {

template<typename Data, typename Allocator>
void sync_buffer(HardwareBuffer::ptr* buffer, Data* data, Allocator* allocator, HardwareBufferPurpose purpose) {
    HardwareBuffer::ptr& target = *buffer;
    if(!target && data->count()) {
        target = allocator->hardware_buffers->allocate(
            data->data_size(),
            purpose,
            SHADOW_BUFFER_DISABLED
        );
        assert(target);
    } else {
        // If we have a buffer, then resize it (possibly to zero)
        if(target) {
            target->resize(data->data_size());
        }

        if(!data->count()) {
            // Just bail out if there is nothing to upload
            return;
        }
    }

    assert(target);
    target->upload(*data);
}

}
