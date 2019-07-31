#include <stdexcept>

#include "../math/quaternion.h"
#include "openal_sound_driver.h"
#include "../logging.h"
#include "al_error.h"

static_assert(sizeof(ALuint) == sizeof(uint32_t), "Unexpected mismatch with AL types");

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

    if(!ctx) {
        L_ERROR("Unable to create sound context");
        return false;
    }

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
    ALCheck(alGenSources, (ALuint) count, (ALuint*) &sources[0]);
    return sources;
}

std::vector<AudioBufferID> OpenALSoundDriver::generate_buffers(uint32_t count) {
    std::vector<AudioBufferID> buffers(count, 0);
    ALCheck(alGenBuffers, (ALuint) count, (ALuint*) &buffers[0]);
    return buffers;
}

void OpenALSoundDriver::delete_buffers(const std::vector<AudioBufferID> &buffers) {
    ALCheck(alDeleteBuffers, (ALuint) buffers.size(), (ALuint*) &buffers[0]);
}

void OpenALSoundDriver::delete_sources(const std::vector<AudioSourceID> &sources) {
    ALCheck(alDeleteSources, (ALuint) sources.size(), (ALuint*) &sources[0]);
}

void OpenALSoundDriver::play_source(AudioSourceID source_id) {
    ALCheck(alSourcePlay, source_id);
}

void OpenALSoundDriver::stop_source(AudioSourceID source_id) {
    ALCheck(alSourceStop, (ALuint) source_id);
}

void OpenALSoundDriver::queue_buffers_to_source(AudioSourceID source, uint32_t count, const std::vector<AudioBufferID> &buffers) {
    ALCheck(alSourceQueueBuffers, (ALuint) source, (ALuint) count, (ALuint*) &buffers[0]);
}

std::vector<AudioBufferID> OpenALSoundDriver::unqueue_buffers_from_source(AudioSourceID source, uint32_t count) {
    std::vector<AudioBufferID> buffers(count, 0);
    ALCheck(alSourceUnqueueBuffers, source, count, (ALuint*) &buffers[0]);
    return buffers;
}

void OpenALSoundDriver::upload_buffer_data(AudioBufferID buffer, AudioDataFormat format, const uint8_t* data, std::size_t bytes, uint32_t frequency) {
    ALenum al_format;

    switch(format) {
    case AUDIO_DATA_FORMAT_MONO8: al_format = AL_FORMAT_MONO8; break;
    case AUDIO_DATA_FORMAT_MONO16: al_format = AL_FORMAT_MONO16; break;
    case AUDIO_DATA_FORMAT_STEREO8: al_format = AL_FORMAT_STEREO8; break;
    case AUDIO_DATA_FORMAT_STEREO16: al_format = AL_FORMAT_STEREO16; break;
    default:
        throw std::runtime_error("Invalid format");
    }

    ALCheck(alBufferData, buffer, al_format, &data[0], bytes, frequency);
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
    ALCheck(alGetSourcei, (ALuint) source, AL_BUFFERS_PROCESSED, (ALint*) &processed);
    return processed;
}

void OpenALSoundDriver::set_source_as_ambient(AudioSourceID id) {
    ALCheck(alSourcei, id, AL_SOURCE_RELATIVE, AL_TRUE);
    ALCheck(alSource3f, id, AL_POSITION, 0, 0, 0);
}

void OpenALSoundDriver::set_listener_properties(const Vec3& position, const Quaternion& orientation, const Vec3& velocity) {
    auto forward = orientation.forward();
    auto up = orientation.up();
    ALfloat ori[] = { forward.x, forward.y, forward.z, up.x, up.y, up.z };
    ALCheck(alListener3f, AL_POSITION, position.x, position.y, position.z);
    ALCheck(alListener3f, AL_VELOCITY, velocity.x, velocity.y, velocity.z);
    ALCheck(alListenerfv, AL_ORIENTATION, ori);
}

void OpenALSoundDriver::set_source_properties(AudioSourceID id, const Vec3& position, const Quaternion& orientation, const Vec3& velocity) {
    auto forward = orientation.forward();
    auto up = orientation.up();
    ALfloat ori[] = { forward.x, forward.y, forward.z, up.x, up.y, up.z };
    ALCheck(alSource3f, id, AL_POSITION, position.x, position.y, position.z);
    ALCheck(alSource3f, id, AL_VELOCITY, velocity.x, velocity.y, velocity.z);
    ALCheck(alSourcefv, id, AL_ORIENTATION, ori);
}

void OpenALSoundDriver::set_source_reference_distance(AudioSourceID id, float dist) {
    ALCheck(alSourcef, id, AL_REFERENCE_DISTANCE, dist);
}

void OpenALSoundDriver::set_source_gain(AudioSourceID id, RangeValue<0, 1> value) {
    ALCheck(alSourcef, id, AL_GAIN, (float) value);
}

void OpenALSoundDriver::set_source_pitch(AudioSourceID id, RangeValue<0, 1> value) {
    ALCheck(alSourcef, id, AL_PITCH, (float) value);
}

}
