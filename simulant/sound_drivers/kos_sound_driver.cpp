#include "kos_sound_driver.h"

namespace smlt {


bool KOSSoundDriver::startup() {
    return true;
}

void KOSSoundDriver::shutdown() {

}

std::vector<AudioSourceID> KOSSoundDriver::generate_sources(uint32_t count) {
    return {};
}

std::vector<AudioBufferID> KOSSoundDriver::generate_buffers(uint32_t count) {
    return {};
}

void KOSSoundDriver::delete_buffers(const std::vector<AudioBufferID>& buffers) {

}

void KOSSoundDriver::delete_sources(const std::vector<AudioSourceID>& sources) {

}

void KOSSoundDriver::play_source(AudioSourceID source_id) {

}

void KOSSoundDriver::stop_source(AudioSourceID source_id) {

}

void KOSSoundDriver::queue_buffers_to_source(AudioSourceID source, uint32_t count, const std::vector<AudioBufferID>& buffers) {

}

std::vector<AudioBufferID> KOSSoundDriver::unqueue_buffers_from_source(AudioSourceID source, uint32_t count) {
    return {};
}

void KOSSoundDriver::upload_buffer_data(AudioBufferID buffer, AudioDataFormat format, int16_t* data, uint32_t size, uint32_t frequency) {

}

AudioSourceState KOSSoundDriver::source_state(AudioSourceID source) {
    return AUDIO_SOURCE_STATE_STOPPED;
}

int32_t KOSSoundDriver::source_buffers_processed_count(AudioSourceID source) const {
    return 0;
}


}
