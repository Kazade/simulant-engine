#pragma once

#if defined(__APPLE__)
#include <OpenAL/al.h>
#include <OpenAL/alc.h>
#else
#include <AL/al.h>
#include <AL/alc.h>
#endif

#include "../sound_driver.h"

namespace smlt {

class OpenALSoundDriver : public SoundDriver {
public:
    OpenALSoundDriver(Window* window):
        SoundDriver(window) {}

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

    void set_source_as_ambient(AudioSourceID id) override;
    void set_listener_properties(const Vec3& position, const Quaternion& orientation, const Vec3& velocity) override;
    void set_source_properties(AudioSourceID id, const Vec3& position, const Quaternion& orientation, const Vec3& velocity) override;

    void set_source_reference_distance(AudioSourceID id, float dist) override;
    void set_source_gain(AudioSourceID id, RangeValue<0, 1> value) override;
    void set_source_pitch(AudioSourceID id, RangeValue<0, 1> value) override;
private:
    ALCdevice* dev = nullptr;
    ALCcontext* ctx = nullptr;
};

}
