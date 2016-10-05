
#include "hardware_buffer.h"
#include "vertex_data.h"

namespace kglt {

HardwareBuffer::ptr HardwareBufferManager::allocate(std::size_t size, HardwareBufferPurpose purpose, HardwareBufferUsage usage) {
    return HardwareBuffer::create(do_allocation(size, purpose, usage));
}

void HardwareBufferImpl::release() {
    manager->do_release(this);
}

HardwareBuffer::HardwareBuffer(std::unique_ptr<HardwareBufferImpl> impl):
    impl_(std::move(impl)) {

}

void HardwareBuffer::upload(IndexData &index_data) {
    assert(0 && "Not Implemented");
}

void HardwareBuffer::upload(VertexData &vertex_data) {
    upload(vertex_data.data(), vertex_data.data_size());
}

void HardwareBuffer::upload(const uint8_t *data, const std::size_t size) {
    if(is_dead()) {
        throw std::logic_error("Tried to reuse a dead hardware buffer");
    }

    if(size > impl_->size) {
        throw std::out_of_range("Tried to upload too much data to a hardware buffer");
    }

    impl_->upload(data, size);
}

}
