#pragma once

#ifdef _arch_dreamcast
#include <GL/gl.h>
#else
#include "./glad/glad/glad.h"
#endif

#include "../../hardware_buffer.h"

namespace smlt {

struct GL1HardwareBufferImpl : public HardwareBufferImpl {
    uint8_t* buffer = nullptr;
    std::size_t size = 0;

    GL1HardwareBufferImpl(HardwareBufferManager* manager):
        HardwareBufferImpl(manager) {}

    GL1HardwareBufferImpl(const GL1HardwareBufferImpl&) = delete;
    GL1HardwareBufferImpl& operator=(GL1HardwareBufferImpl&) = delete;

    ~GL1HardwareBufferImpl() {

    }

    void upload(const uint8_t *data, const std::size_t size) override {

    }
};

class GL1BufferManager:
    public smlt::HardwareBufferManager {

public:
    GL1BufferManager(const Renderer* renderer);

private:
    std::unique_ptr<HardwareBufferImpl> do_allocation(std::size_t size, HardwareBufferPurpose purpose, HardwareBufferUsage usage);
    void do_release(const HardwareBufferImpl *buffer);
    void do_resize(HardwareBufferImpl* buffer, std::size_t new_size);
    void do_bind(const HardwareBufferImpl *buffer, HardwareBufferPurpose purpose);
};

}
