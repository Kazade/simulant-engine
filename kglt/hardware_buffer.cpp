
#include "hardware_buffer.h"
#include "vertex_data.h"

namespace kglt {

HardwareBufferManager::HardwareBufferManager(const Renderer* renderer):
    renderer_(renderer) {

}

HardwareBuffer::ptr HardwareBufferManager::allocate(std::size_t size, HardwareBufferPurpose purpose, HardwareBufferUsage usage) {
    return HardwareBuffer::ptr(new HardwareBuffer(do_allocation(size, purpose, usage)));
}

void HardwareBufferImpl::resize(std::size_t new_size) {
    manager->do_resize(this, new_size);
}

void HardwareBufferImpl::release() {
    manager->do_release(this);
}

void HardwareBufferImpl::bind(HardwareBufferPurpose purpose) {
    manager->do_bind(this, purpose);
}

HardwareBuffer::HardwareBuffer(std::unique_ptr<HardwareBufferImpl> impl):
    impl_(std::move(impl)) {

}

void HardwareBuffer::upload(IndexData &index_data) {
    upload(index_data.data(), index_data.data_size());
}

void HardwareBuffer::upload(VertexData &vertex_data) {
    upload(vertex_data.data(), vertex_data.data_size());
}

void HardwareBuffer::bind(HardwareBufferPurpose purpose) {
    impl_->bind(purpose);
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
