#pragma once

#include "./glad/glad/glad.h"
#include "../../hardware_buffer.h"

namespace kglt {

struct GL2HardwareBufferImpl : public HardwareBufferImpl {
    GLuint buffer_id; // The ID of the VBO we are using
    std::size_t offset; // The offset into the VBO that this hardware buffer begins
    GLenum usage; // The usage of this buffer
    GLenum purpose; // The purpose of this buffer

    GL2HardwareBufferImpl(HardwareBufferManager* manager):
        HardwareBufferImpl(manager) {}

    void upload(const uint8_t *data, const std::size_t size) override;
};

class GL2BufferManager:
    public kglt::HardwareBufferManager {

public:
    GL2BufferManager(const Renderer* renderer);

private:
    std::unique_ptr<HardwareBufferImpl> do_allocation(std::size_t size, HardwareBufferPurpose purpose, HardwareBufferUsage usage);
    void do_release(const HardwareBufferImpl *buffer);
    void do_resize(HardwareBufferImpl* buffer, std::size_t new_size);
    void do_bind(const HardwareBufferImpl *buffer, HardwareBufferPurpose purpose);
};

}
