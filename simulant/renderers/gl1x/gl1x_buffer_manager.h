#pragma once

#ifdef _arch_dreamcast
#include "../../../deps/libgl/include/gl.h"
#else
#include "./glad/glad/glad.h"
#endif

#include "../../hardware_buffer.h"

#include <vector>
#include <cstdint>

namespace smlt {

struct GL1HardwareBufferImpl : public HardwareBufferImpl {
    GL1HardwareBufferImpl(HardwareBufferManager* manager):
        HardwareBufferImpl(manager) {}

    GL1HardwareBufferImpl(const GL1HardwareBufferImpl&) = delete;
    GL1HardwareBufferImpl& operator=(GL1HardwareBufferImpl&) = delete;

    ~GL1HardwareBufferImpl() {

    }

    void upload(const uint8_t *data, const std::size_t size) override {
        // Make sure we can fit the data
        resize(size);

        // If we have a shadow buffer, upload to there, otherwise, load into the target buffer
        std::vector<uint8_t>* target = (has_shadow_buffer()) ? &shadow_buffer_ : &target_buffer_;
        target->assign(data, data + (size * sizeof(uint8_t)));
    }

    bool has_shadow_buffer() const override { return has_shadow_buffer_; }
    BufferLocation shadow_buffer_location() const override { return BUFFER_LOCATION_RAM; }
    BufferLocation target_buffer_location() const override { return BUFFER_LOCATION_RAM; }
    void update_target_from_shadow_buffer() override;

    void destroy_shadow_buffer() override;

    MappedBuffer map_target_for_read() const {
        return MappedBuffer(
            [this]() -> uint8_t* { return &target_buffer_[0]; },
            []() {}
        );
    }

private:
    friend class GL1BufferManager;

    bool has_shadow_buffer_ = false;

    // mutable as it's an implementation detail that this is here rather than on the manager
    mutable std::vector<uint8_t> target_buffer_;
    mutable std::vector<uint8_t> shadow_buffer_; // Rarely used
};

class GL1BufferManager:
    public smlt::HardwareBufferManager {

public:
    GL1BufferManager(const Renderer* renderer);

private:
    std::unique_ptr<HardwareBufferImpl> do_allocation(
        std::size_t size,
        HardwareBufferPurpose purpose,
        ShadowBufferEnableOption shadow_buffer,
        HardwareBufferUsage usage
    );

    void do_release(const HardwareBufferImpl *buffer);
    void do_resize(HardwareBufferImpl* buffer, std::size_t new_size);
    void do_bind(const HardwareBufferImpl *buffer, HardwareBufferPurpose purpose);
};

}
