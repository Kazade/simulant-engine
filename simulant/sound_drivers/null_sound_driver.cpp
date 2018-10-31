#include "null_sound_driver.h"

namespace smlt {

NullSoundDriver::~NullSoundDriver() {

}

bool NullSoundDriver::startup() {
    return true;
}

void NullSoundDriver::shutdown() {

}

std::vector<AudioSourceID> NullSoundDriver::generate_sources(uint32_t count) {
    std::vector<AudioSourceID> ret;
    for(auto i = 0u; i < count; ++i) {
        ret.push_back(++source_counter_);
    }
    return ret;
}

std::vector<AudioBufferID> NullSoundDriver::generate_buffers(uint32_t count) {
    std::vector<AudioBufferID> ret;
    for(auto i = 0u; i < count; ++i) {
        ret.push_back(++buffer_counter_);
    }
    return ret;
}

void NullSoundDriver::delete_buffers(const std::vector<AudioBufferID>& buffers) {

}

void NullSoundDriver::delete_sources(const std::vector<AudioSourceID>& sources) {
    for(auto& src: sources) {
        source_playing_.erase(src);
    }
}

void NullSoundDriver::play_source(AudioSourceID source_id) {
    source_playing_[source_id] = true;
}

void NullSoundDriver::stop_source(AudioSourceID source_id) {
    source_playing_[source_id] = false;
}

void NullSoundDriver::queue_buffers_to_source(AudioSourceID source, uint32_t count, const std::vector<AudioBufferID>& buffers) {

}

std::vector<AudioBufferID> NullSoundDriver::unqueue_buffers_from_source(AudioSourceID source, uint32_t count) {
    return {};
}

void NullSoundDriver::upload_buffer_data(AudioBufferID buffer, AudioDataFormat format, int16_t* data, uint32_t size, uint32_t frequency) {

}

AudioSourceState NullSoundDriver::source_state(AudioSourceID source) {
    if(source_playing_.at(source)) {
        source_playing_[source] = false;
        return AUDIO_SOURCE_STATE_PLAYING;
    }

    return AUDIO_SOURCE_STATE_STOPPED;
}

int32_t NullSoundDriver::source_buffers_processed_count(AudioSourceID source) const {
    return 0;
}


}
