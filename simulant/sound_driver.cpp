#include "application.h"
#include "window.h"

#include "sound_driver.h"
#include "sound.h"

namespace smlt {

SoundDriver::SoundDriver(Window *window):
    window_(window) {


}

SoundDriver::~SoundDriver() {
    source_update_.disconnect();
}

bool SoundDriver::startup() {
    return _startup();
}

void SoundDriver::shutdown() {
    _shutdown();
}

bool SoundDriver::can_persist_buffers(size_t sample_data_size) const {
    /* Return true if the sound driver can persist the entire
     * sound in audio ram. */
    return false;
}

size_t SoundDriver::max_buffer_size() const {
    // FIXME: Move this to NVI which would make the max_buffer_size_
    // caching less brittle.
    if(max_buffer_size_cache_ != 0) {
        return max_buffer_size_cache_;
    }

    size_t default_size = 8 * 1024;
    size_t max = 128 * 1024;
    auto buffers = const_cast<SoundDriver*>(this)->generate_buffers(1);
    if(buffers.empty()) {
        // Couldn't generate a buffer, something went wrong.
        S_ERROR("Couldn't generate an audio buffer to test max size. "
                "Defaulting to 8kb");
        max_buffer_size_cache_ = default_size;
        return default_size;
    }
    std::vector<uint8_t> data(max);
    while(max > 0) {
        bool success = const_cast<SoundDriver*>(this)->upload_buffer_data(
            buffers[0], AUDIO_DATA_FORMAT_MONO8, &data[0], max, 44100);

        if(success) {
            const_cast<SoundDriver*>(this)->destroy_buffers(buffers);
            max_buffer_size_cache_ = max;
            return max;
        }

        max /= 2;
    }

    const_cast<SoundDriver*>(this)->destroy_buffers(buffers);
    S_WARN("Error determining max buffersize. Returning default.");
    max_buffer_size_cache_ = default_size;
    return default_size;
}
}
