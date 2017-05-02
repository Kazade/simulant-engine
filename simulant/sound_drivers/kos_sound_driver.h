#pragma once

#include "sound_driver.h"

namespace smlt {

class KOSSoundDriver : public SoundDriver {
public:
    bool startup() override;
    void shutdown() override;

    std::vector<AudioSourceID> generate_sources(uint32_t count) override;
    std::vector<AudioBufferID> generate_buffers(uint32_t count) override;

    void delete_buffers(const std::vector<AudioBufferID>& buffers) override;
    void delete_sources(const std::vector<AudioSourceID>& sources) override;

    void play_source(AudioSourceID source_id) override;
    void stop_source(AudioSourceID source_id) override;

    void queue_buffers_to_source(AudioSourceID source, uint32_t count, const std::vector<AudioBufferID>& buffers) override;
    void unqueue_buffers_from_source(AudioSourceID source, uint32_t count, const std::vector<AudioBufferID>& buffers) override;
    void upload_buffer_data(AudioBufferID buffer, AudioDataFormat format, int16_t* data, uint32_t size, uint32_t frequency) override;

    AudioSourceState source_state(AudioSourceID source) override;
    int32_t source_buffers_processed_count(AudioSourceID source) const override;
};

}
