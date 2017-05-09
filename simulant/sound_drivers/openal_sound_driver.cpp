#include "openal_sound_driver.h"
#include "../deps/kazlog/kazlog.h"
#include "../utils/al_error.h"

namespace smlt {

bool OpenALSoundDriver::startup() {
    dev = alcOpenDevice(NULL);
    if(!dev) {
        // If we couldn't open the default device for some reason,
        // disable all sound functions
        L_ERROR("Unable to initialize sound device");
        return false;
    }

    ctx = alcCreateContext(dev, NULL);
    alcMakeContextCurrent(ctx);

    return true;
}

void OpenALSoundDriver::shutdown() {
    if(ctx) {
        alcDestroyContext(ctx);
        ctx = nullptr;
    }

    if(dev) {
        alcCloseDevice(dev);
        dev = nullptr;
    }
}

std::vector<AudioSourceID> OpenALSoundDriver::generate_sources(uint32_t count) {
    std::vector<AudioSourceID> sources(count, 0);
    ALCheck(alGenSources, count, &sources[0]);
    return sources;
}

std::vector<AudioBufferID> OpenALSoundDriver::generate_buffers(uint32_t count) {
    std::vector<AudioBufferID> buffers(count, 0);
    ALCheck(alGenBuffers, count, &buffers[0]);
    return buffers;
}

void OpenALSoundDriver::delete_buffers(const std::vector<AudioBufferID> &buffers) {
    ALCheck(alDeleteBuffers, buffers.size(), &buffers[0]);
}

void OpenALSoundDriver::delete_sources(const std::vector<AudioSourceID> &sources) {
    ALCheck(alDeleteSources, sources.size(), &sources[0]);
}

void OpenALSoundDriver::play_source(AudioSourceID source_id) {
    ALCheck(alSourcePlay, source_id);
}

void OpenALSoundDriver::stop_source(AudioSourceID source_id) {
    ALCheck(alSourceStopv, 1, &source_id);
}

void OpenALSoundDriver::queue_buffers_to_source(AudioSourceID source, uint32_t count, const std::vector<AudioBufferID> &buffers) {
    ALCheck(alSourceQueueBuffers, source, count, &buffers[0]);
}

std::vector<AudioBufferID> OpenALSoundDriver::unqueue_buffers_from_source(AudioSourceID source, uint32_t count) {
    std::vector<AudioBufferID> buffers(count, 0);
    ALCheck(alSourceUnqueueBuffers, source, count, (ALuint*) &buffers[0]);
    return buffers;
}

void OpenALSoundDriver::upload_buffer_data(AudioBufferID buffer, AudioDataFormat format, int16_t *data, uint32_t size, uint32_t frequency) {
    ALenum al_format;

    switch(format) {
    case AUDIO_DATA_FORMAT_MONO8: al_format = AL_FORMAT_MONO8; break;
    case AUDIO_DATA_FORMAT_MONO16: al_format = AL_FORMAT_MONO16; break;
    case AUDIO_DATA_FORMAT_STEREO8: al_format = AL_FORMAT_STEREO8; break;
    case AUDIO_DATA_FORMAT_STEREO16: al_format = AL_FORMAT_STEREO16; break;
    default:
        throw std::runtime_error("Invalid format");
    }

    ALCheck(alBufferData, buffer, al_format, &data[0], size * sizeof(int16_t), frequency);
}

AudioSourceState OpenALSoundDriver::source_state(AudioSourceID source) {
    ALint val;
    ALCheck(alGetSourcei, source, AL_SOURCE_STATE, &val);

    switch(val) {
    case AL_PLAYING: return AUDIO_SOURCE_STATE_PLAYING;
    case AL_PAUSED: return AUDIO_SOURCE_STATE_PAUSED;
    case AL_STOPPED: return AUDIO_SOURCE_STATE_STOPPED;
    default:
        throw std::runtime_error("Unknown state");
    }
}

int32_t OpenALSoundDriver::source_buffers_processed_count(AudioSourceID source) const {
    int32_t processed = 0;
    ALCheck(alGetSourcei, source, AL_BUFFERS_PROCESSED, &processed);
    return processed;
}

}
