//
//   Copyright (c) 2011-2017 Luke Benstead https://simulant-engine.appspot.com
//
//     This file is part of Simulant.
//
//     Simulant is free software: you can redistribute it and/or modify
//     it under the terms of the GNU Lesser General Public License as published by
//     the Free Software Foundation, either version 3 of the License, or
//     (at your option) any later version.
//
//     Simulant is distributed in the hope that it will be useful,
//     but WITHOUT ANY WARRANTY; without even the implied warranty of
//     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//     GNU Lesser General Public License for more details.
//
//     You should have received a copy of the GNU Lesser General Public License
//     along with Simulant.  If not, see <http://www.gnu.org/licenses/>.
//


#include "hardware_buffer.h"
#include "vertex_data.h"

namespace smlt {

HardwareBufferManager::HardwareBufferManager(const Renderer* renderer):
    renderer_(renderer) {

}

HardwareBuffer::ptr HardwareBufferManager::allocate(std::size_t size, HardwareBufferPurpose purpose, ShadowBufferEnableOption shadow_buffer, HardwareBufferUsage usage) {
    return HardwareBuffer::ptr(new HardwareBuffer(do_allocation(size, purpose, shadow_buffer, usage)));
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

MappedBuffer HardwareBuffer::map_target_for_read() const {
    return impl_->map_target_for_read();
}

}
