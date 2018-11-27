/* *   Copyright (c) 2011-2017 Luke Benstead https://simulant-engine.appspot.com
 *
 *     This file is part of Simulant.
 *
 *     Simulant is free software: you can redistribute it and/or modify
 *     it under the terms of the GNU Lesser General Public License as published by
 *     the Free Software Foundation, either version 3 of the License, or
 *     (at your option) any later version.
 *
 *     Simulant is distributed in the hope that it will be useful,
 *     but WITHOUT ANY WARRANTY; without even the implied warranty of
 *     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *     GNU Lesser General Public License for more details.
 *
 *     You should have received a copy of the GNU Lesser General Public License
 *     along with Simulant.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include <vector>

#include "generic/managed.h"
#include "generic/property.h"

namespace smlt {

class VertexData;
class IndexData;
class Renderer;

enum HardwareBufferPurpose {
    HARDWARE_BUFFER_VERTEX_ATTRIBUTES,
    HARDWARE_BUFFER_VERTEX_ARRAY_INDICES,
    HARDWARE_BUFFER_TEXTURE_DATA
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

/*
 * When a shadow buffer is enabled, writes to the hardware buffer go there and are essentially queued until
 * update_target_from_shadow_buffer() is called. Shadow buffers are normally in RAM, and target buffers on the GPU
 * but on platforms that don't support that (e.g. the Dreamcast) the target buffer may be in RAM (for vertex data)
 * if that's the case, a shadow buffer is wasteful so there is an option to only use a shadow buffer if it makes sense.
 * update_target_from_shadow_buffer() is a no-op in this case.
 */

enum ShadowBufferEnableOption {
    SHADOW_BUFFER_DISABLED, // No shadow buffer, all writes go direct to the target buffer
    SHADOW_BUFFER_ENABLE_IF_OPTIMAL, // Shadow buffer if it makes sense (e.g. target buffer is on GPU, so shadow buffer in RAM)
    SHADOW_BUFFER_ENABLE_REQUIRED // Always create a shadow buffer, even if both shadow and target are in RAM
};

enum BufferLocation {
    BUFFER_LOCATION_RAM,
    BUFFER_LOCATION_VRAM
};


class HardwareBufferManager;

typedef std::function<const uint8_t* ()> MapBufferFunc;
typedef std::function<void ()> UnMapBufferFunc;

class MappedBuffer {
public:
    MappedBuffer(MapBufferFunc map, UnMapBufferFunc unmap):
        buffer_(map()),
        unmap_(unmap) {

    }

    MappedBuffer(MappedBuffer&& rhs):
        buffer_(rhs.buffer_),
        unmap_(rhs.unmap_) {

        rhs.buffer_ = nullptr;
        rhs.unmap_ = UnMapBufferFunc();
    }

    operator const uint8_t*() const {
        return buffer_;
    }

    ~MappedBuffer() {
        unmap_();
        buffer_ = nullptr;
    }

    MappedBuffer(const MappedBuffer& rhs) = delete;
    MappedBuffer& operator=(const MappedBuffer& rhs) = delete;

private:
    const uint8_t* buffer_ = nullptr;
    UnMapBufferFunc unmap_;
};


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

    virtual bool has_shadow_buffer() const = 0;
    virtual BufferLocation shadow_buffer_location() const = 0;
    virtual BufferLocation target_buffer_location() const = 0;
    virtual void update_target_from_shadow_buffer() = 0;
    virtual void destroy_shadow_buffer() = 0;
    virtual MappedBuffer map_target_for_read() const = 0;

    virtual void upload(const uint8_t* data, const std::size_t size) = 0;
};

/* Public-facing API to hardware buffers */
class HardwareBuffer {
public:
    typedef std::unique_ptr<HardwareBuffer> ptr;

    HardwareBuffer(std::unique_ptr<HardwareBufferImpl> impl);
    ~HardwareBuffer() { release(); }

    bool has_shadow_buffer() const;
    BufferLocation shadow_buffer_location() const;
    BufferLocation target_buffer_location() const;
    void update_target_from_shadow_buffer();
    void destroy_shadow_buffer();

    void upload(VertexData& vertex_data);
    void upload(IndexData& index_data);
    void upload(const uint8_t* data, const std::size_t size);

    // Download data from the shadow buffer (if there is one) else the target
    // buffer (may be slow!)
    std::vector<uint8_t> download() const;

    // Map the target buffer for reading
    MappedBuffer map_target_for_read() const;

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
    HardwareBuffer::ptr allocate(
        std::size_t size,
        HardwareBufferPurpose purpose,
        ShadowBufferEnableOption shadow_buffer,
        HardwareBufferUsage usage = HARDWARE_BUFFER_MODIFY_ONCE_USED_FOR_RENDERING
    );

    Property<HardwareBufferManager, const Renderer> renderer = { this, &HardwareBufferManager::renderer_ };
private:
    const Renderer* renderer_;

    virtual std::unique_ptr<HardwareBufferImpl> do_allocation(
        std::size_t size,
        HardwareBufferPurpose purpose,
        ShadowBufferEnableOption shadow_buffer,
        HardwareBufferUsage usage
    ) = 0;

    virtual void do_resize(HardwareBufferImpl* buffer, std::size_t new_size) = 0;
    virtual void do_release(const HardwareBufferImpl* buffer) = 0;
    virtual void do_bind(const HardwareBufferImpl* buffer, HardwareBufferPurpose purpose) = 0;

    friend struct HardwareBufferImpl;
};

}
