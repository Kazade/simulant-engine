#include "null_sound_driver.h"
#include "../../macros.h"

namespace smlt {

NullSoundDriver::~NullSoundDriver() {

}

bool NullSoundDriver::_startup() {
    return true;
}

void NullSoundDriver::_shutdown() {

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

void NullSoundDriver::destroy_buffers(const std::vector<AudioBufferID>& buffers) {
    _S_UNUSED(buffers);
}

void NullSoundDriver::destroy_sources(const std::vector<AudioSourceID>& sources) {
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
    _S_UNUSED(source);
    _S_UNUSED(count);
    _S_UNUSED(buffers);
}

std::vector<AudioBufferID> NullSoundDriver::unqueue_buffers_from_source(AudioSourceID source, uint32_t count) {
    _S_UNUSED(source);
    _S_UNUSED(count);

    return {};
}

void NullSoundDriver::upload_buffer_data(AudioBufferID buffer, AudioDataFormat format, const uint8_t* data, std::size_t bytes, uint32_t frequency) {
    _S_UNUSED(buffer);
    _S_UNUSED(format);
    _S_UNUSED(data);
    _S_UNUSED(bytes);
    _S_UNUSED(frequency);
}

AudioSourceState NullSoundDriver::source_state(AudioSourceID source) {
    if(source_playing_.at(source)) {
        source_playing_[source] = false;
        return AUDIO_SOURCE_STATE_PLAYING;
    }

    return AUDIO_SOURCE_STATE_STOPPED;
}

int32_t NullSoundDriver::source_buffers_processed_count(AudioSourceID source) const {
    _S_UNUSED(source);
    return 0;
}

void NullSoundDriver::set_source_reference_distance(AudioSourceID id, float dist) {
    _S_UNUSED(id);
    _S_UNUSED(dist);
}

void NullSoundDriver::set_source_pitch(AudioSourceID id, RangeValue<0, 1> value) {
    _S_UNUSED(id);
    _S_UNUSED(value);
}

void NullSoundDriver::set_source_gain(AudioSourceID id, RangeValue<0, 1> value) {
    _S_UNUSED(id);
    _S_UNUSED(value);
}


}
