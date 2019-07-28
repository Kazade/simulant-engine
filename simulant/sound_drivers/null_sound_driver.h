#pragma once

#include <map>

#include "../sound_driver.h"

namespace smlt {

class NullSoundDriver : public SoundDriver {
public:
    NullSoundDriver(Window* window):
        SoundDriver(window) {}

    virtual ~NullSoundDriver();

    bool startup() override;
    void shutdown() override;

    std::vector<AudioSourceID> generate_sources(uint32_t count) override;
    std::vector<AudioBufferID> generate_buffers(uint32_t count) override;

    void delete_buffers(const std::vector<AudioBufferID>& buffers) override;
    void delete_sources(const std::vector<AudioSourceID>& sources) override;

    void play_source(AudioSourceID source_id) override;
    void stop_source(AudioSourceID source_id) override;

    void queue_buffers_to_source(AudioSourceID source, uint32_t count, const std::vector<AudioBufferID>& buffers) override;
    std::vector<AudioBufferID> unqueue_buffers_from_source(AudioSourceID source, uint32_t count) override;
    void upload_buffer_data(AudioBufferID buffer, AudioDataFormat format, const uint8_t* data, std::size_t bytes, uint32_t frequency) override;

    AudioSourceState source_state(AudioSourceID source) override;
    int32_t source_buffers_processed_count(AudioSourceID source) const override;

private:
    AudioSourceID source_counter_ = 0;
    AudioBufferID buffer_counter_ = 0;

    std::map<AudioSourceID, bool> source_playing_;
};

}
