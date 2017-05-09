#pragma once

#include <cstdint>
#include <vector>

#include "../generic/property.h"

namespace smlt {

typedef uint32_t AudioSourceID;
typedef uint32_t AudioBufferID;


enum AudioSourceState {
    AUDIO_SOURCE_STATE_PLAYING,
    AUDIO_SOURCE_STATE_PAUSED,
    AUDIO_SOURCE_STATE_STOPPED
};

enum AudioDataFormat {
    AUDIO_DATA_FORMAT_MONO8,
    AUDIO_DATA_FORMAT_MONO16,
    AUDIO_DATA_FORMAT_STEREO8,
    AUDIO_DATA_FORMAT_STEREO16
};


class WindowBase;

/* Basically a hacky abstraction over OpenAL with the thinking that the only other drivers will be:
 *
 * - Dreamcast
 * - Dummy
 *
 * If that ceases to be the case for whatever reason, then we should probably design a nicer API for this. Perhaps.
 */
class SoundDriver {
public:
    SoundDriver(WindowBase* window):
        window_(window) {}

    virtual ~SoundDriver() {}

    virtual bool startup() = 0;
    virtual void shutdown() = 0;

    virtual std::vector<AudioSourceID> generate_sources(uint32_t count) = 0;
    virtual std::vector<AudioBufferID> generate_buffers(uint32_t count) = 0;

    virtual void delete_buffers(const std::vector<AudioBufferID>& buffers) = 0;
    virtual void delete_sources(const std::vector<AudioSourceID>& sources) = 0;

    virtual void play_source(AudioSourceID source_id) = 0;
    virtual void stop_source(AudioSourceID source_id) = 0;

    virtual void queue_buffers_to_source(AudioSourceID source, uint32_t count, const std::vector<AudioBufferID>& buffers) = 0;
    virtual std::vector<AudioBufferID> unqueue_buffers_from_source(AudioSourceID source, uint32_t count) = 0;
    virtual void upload_buffer_data(AudioBufferID buffer, AudioDataFormat format, int16_t* data, uint32_t size, uint32_t frequency) = 0;

    virtual AudioSourceState source_state(AudioSourceID source) = 0;
    virtual int32_t source_buffers_processed_count(AudioSourceID source) const = 0;

    Property<SoundDriver, WindowBase> window = {this, &SoundDriver::window_};

private:
    WindowBase* window_ = nullptr;
};


}
