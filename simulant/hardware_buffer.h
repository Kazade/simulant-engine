/* *   Copyright (c) 2011-2017 Luke Benstead https://simulant-engine.appspot.com
 *
 *     This file is part of Simulant.
 *
 *     Simulant is free software: you can redistribute it and/or modify
 *     it under the terms of the GNU General Public License as published by
 *     the Free Software Foundation, either version 3 of the License, or
 *     (at your option) any later version.
 *
 *     Simulant is distributed in the hope that it will be useful,
 *     but WITHOUT ANY WARRANTY; without even the implied warranty of
 *     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *     GNU General Public License for more details.
 *
 *     You should have received a copy of the GNU General Public License
 *     along with Simulant.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include "generic/managed.h"
#include "generic/property.h"

namespace smlt {

class VertexData;
class IndexData;
class Renderer;

enum HardwareBufferPurpose {
    HARDWARE_BUFFER_VERTEX_ATTRIBUTES,
    HARDWARE_BUFFER_VERTEX_ARRAY_INDICES
};

enum HardwareBufferUsage {
    HARDWARE_BUFFER_MODIFY_ONCE_USED_FOR_LIMITED_RENDERING,
    HARDWARE_BUFFER_MODIFY_ONCE_USED_FOR_LIMITED_QUERYING,
    HARDWARE_BUFFER_MODIFY_ONCE_USED_FOR_LIMITED_QUERYING_AND_RENDERING,
    HARDWARE_BUFFER_MODIFY_ONCE_USED_FOR_RENDERING,
    HARDWARE_BUFFER_MODIFY_ONCE_USED_FOR_QUERYING,
    HARDWARE_BUFFER_MODIFY_ONCE_USED_FOR_QUERYING_AND_RENDERING,
    HARDWARE_BUFFER_MODIFY_REPEATEDLY_USED_FOR_RENDERING,
    HARDWARE_BUFFER_MODIFY_REPEATEDLY_USED_FOR_QUERYING,
    HARDWARE_BUFFER_MODIFY_REPEATEDLY_USED_FOR_QUERYING_AND_RENDERING
};

class HardwareBufferManager;

/* HardwareBufferManager subclasses should return their own subclass of this
 * and use it to store additional data (e.g. GL buffer IDs or whatever)
 */
struct HardwareBufferImpl {
    HardwareBufferManager* manager = nullptr;
    std::size_t capacity = 0; ///< This is the actual amount of allocated memory
    std::size_t size = 0; ///< The size of the buffer, this is for bounds checking and...
    // ... used when rendering

    HardwareBufferImpl(HardwareBufferManager* manager):
        manager(manager) {
    }
    virtual ~HardwareBufferImpl() {}

    void bind(HardwareBufferPurpose purpose);
    void resize(std::size_t new_size);
    void release();

    virtual void upload(const uint8_t* data, const std::size_t size) = 0;
};


/* Public-facing API to hardware buffers */
class HardwareBuffer {
public:
    typedef std::unique_ptr<HardwareBuffer> ptr;

    HardwareBuffer(std::unique_ptr<HardwareBufferImpl> impl);
    ~HardwareBuffer() { release(); }

    void upload(VertexData& vertex_data);
    void upload(IndexData& index_data);
    void upload(const uint8_t* data, const std::size_t size);

    void bind(HardwareBufferPurpose purpose);

    void resize(std::size_t new_size) {
        impl_->resize(new_size);
    }

    void release() {
        if(impl_) {
            impl_->release();
            impl_.reset();
        }
        is_dead_ = true;
    }

    std::size_t size() const { return impl_->size; }
    bool is_dead() const { return is_dead_; }

    // Make sure we can't copy hardware buffers, they must be passed around as unique_ptr<HardwareBuffer>
    HardwareBuffer(const HardwareBuffer&) = delete;
    HardwareBuffer& operator=(HardwareBuffer& rhs) const = delete;

private:
    std::unique_ptr<HardwareBufferImpl> impl_;
    bool is_dead_ = false;
};


/* Public-facing API for allocating buffers, all renderers should have a "hardware_buffers" property
 * that returns a HardwareBufferManager subclass */
class HardwareBufferManager {
public:
    HardwareBufferManager(const Renderer* renderer);
    HardwareBuffer::ptr allocate(std::size_t size, HardwareBufferPurpose purpose, HardwareBufferUsage usage=HARDWARE_BUFFER_MODIFY_ONCE_USED_FOR_RENDERING);

    Property<HardwareBufferManager, const Renderer> renderer = { this, &HardwareBufferManager::renderer_ };
private:
    const Renderer* renderer_;

    virtual std::unique_ptr<HardwareBufferImpl> do_allocation(std::size_t size, HardwareBufferPurpose purpose, HardwareBufferUsage usage) = 0;

    virtual void do_resize(HardwareBufferImpl* buffer, std::size_t new_size) = 0;
    virtual void do_release(const HardwareBufferImpl* buffer) = 0;
    virtual void do_bind(const HardwareBufferImpl* buffer, HardwareBufferPurpose purpose) = 0;

    friend struct HardwareBufferImpl;
};

}
