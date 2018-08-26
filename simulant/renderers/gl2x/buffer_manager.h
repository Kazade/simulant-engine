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

#include "../glad/glad/glad.h"
#include "../../hardware_buffer.h"

namespace smlt {

struct GL2HardwareBufferImpl : public HardwareBufferImpl {
    GLuint buffer_id; // The ID of the VBO we are using
    std::size_t offset; // The offset into the VBO that this hardware buffer begins
    GLenum usage; // The usage of this buffer
    GLenum purpose; // The purpose of this buffer

    GL2HardwareBufferImpl(HardwareBufferManager* manager):
        HardwareBufferImpl(manager) {}

    void upload(const uint8_t *data, const std::size_t size) override;

    virtual bool has_shadow_buffer() const override { return has_shadow_buffer_; }
    virtual BufferLocation shadow_buffer_location() const override { return BUFFER_LOCATION_RAM; }
    virtual BufferLocation target_buffer_location() const override { return BUFFER_LOCATION_VRAM; }
    virtual void update_target_from_shadow_buffer() override {
        //fixme: IMPLEMENT!
    }

    virtual void destroy_shadow_buffer() override  {
        has_shadow_buffer_ = false;
        shadow_buffer_.clear();
    }

    virtual MappedBuffer map_target_for_read() const {
        //FIXME: implement
        return MappedBuffer(
            []() -> uint8_t* { return nullptr; },
            [](){}
        );
    }
private:
    bool has_shadow_buffer_ = false;
    mutable std::vector<uint8_t> shadow_buffer_; // Rarely used

    friend class GL2BufferManager;
};

class GL2BufferManager:
    public smlt::HardwareBufferManager {

public:
    GL2BufferManager(const Renderer* renderer);

private:
    std::unique_ptr<HardwareBufferImpl> do_allocation(std::size_t size, HardwareBufferPurpose purpose, ShadowBufferEnableOption shadow_buffer, HardwareBufferUsage usage);
    void do_release(const HardwareBufferImpl *buffer);
    void do_resize(HardwareBufferImpl* buffer, std::size_t new_size);
    void do_bind(const HardwareBufferImpl *buffer, HardwareBufferPurpose purpose);
};

}
