#ifdef _arch_dreamcast
#include <kos.h>
#endif
#include "../../deps/kazlog/kazlog.h"
#include "../../utils/memory.h"
#include "gl1x_buffer_manager.h"

namespace smlt {

GL1BufferManager::GL1BufferManager(const Renderer *renderer):
    HardwareBufferManager(renderer) {

}


std::unique_ptr<HardwareBufferImpl> GL1BufferManager::do_allocation(
    std::size_t size,
    HardwareBufferPurpose purpose,
    ShadowBufferEnableOption shadow_buffer,
    HardwareBufferUsage usage
) {

    L_DEBUG(_F("Allocating HW buffer of size {0}").format(size));

#ifdef _arch_dreamcast
    print_available_ram();
#endif

    std::unique_ptr<GL1HardwareBufferImpl> buffer_impl(new GL1HardwareBufferImpl(this));
    buffer_impl->target_buffer_.resize(size, 0);
    buffer_impl->size = buffer_impl->capacity = size;

    // Only create a shadow buffer if it was required as we don't use VRAM for our target
    // buffer location, it is also in RAM
    if(shadow_buffer == SHADOW_BUFFER_ENABLE_REQUIRED) {
        buffer_impl->shadow_buffer_.resize(size, 0);
        buffer_impl->has_shadow_buffer_ = true;
    }

    return std::move(buffer_impl);
}

void GL1BufferManager::do_release(const HardwareBufferImpl *buffer) {
    L_DEBUG("Releasing HW buffer");

    const GL1HardwareBufferImpl* impl = static_cast<const GL1HardwareBufferImpl*>(buffer);

    impl->target_buffer_.clear();
    impl->shadow_buffer_.clear();
}

void GL1BufferManager::do_resize(HardwareBufferImpl* buffer, std::size_t new_size) {
    const GL1HardwareBufferImpl* impl = static_cast<const GL1HardwareBufferImpl*>(buffer);

    impl->target_buffer_.resize(new_size, 0);
    if(impl->has_shadow_buffer()) {
        impl->shadow_buffer_.resize(new_size, 0);
    }
}

void GL1BufferManager::do_bind(const HardwareBufferImpl *buffer, HardwareBufferPurpose purpose) {
    // RAM buffer, no need to do anything
}

void GL1HardwareBufferImpl::update_target_from_shadow_buffer() {
    target_buffer_.assign(shadow_buffer_.begin(), shadow_buffer_.end());
}

void GL1HardwareBufferImpl::destroy_shadow_buffer() {
    shadow_buffer_.clear();
    has_shadow_buffer_ = false;
}

}
