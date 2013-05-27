#include "subscene.h"
#include "sound.h"

namespace kglt {

static ALCdevice* dev = nullptr;
static ALCcontext* ctx = nullptr;

void Sound::init_openal() {
    if(!dev) {
        dev = alcOpenDevice(NULL);
        ctx = alcCreateContext(dev, NULL);
        alcMakeContextCurrent(ctx);
    }
}

void Sound::shutdown_openal() {
    alcDestroyContext(ctx);
    alcCloseDevice(dev);
}

Sound::Sound(ResourceManager *resource_manager, SoundID id):
    generic::Identifiable<SoundID>(id),
    Resource(resource_manager) {


}

Source::Source(SubScene& subscene):
    subscene_(subscene) {


}

Source::~Source() {
    alDeleteSources(1, &al_source_);
    alDeleteBuffers(2, buffers_);
}

void Source::play(SoundID sound_id, bool loop) {
    SoundPtr sound = subscene_.sound(sound_id).lock();
    sound->init_source_(*this);

    if(!al_source_) {
        alGenSources(1, &al_source_);
    }

    if(!buffers_[0]) {
        alGenBuffers(2, buffers_);
    }

    //Fill up two buffers to begin with
    stream_func_(buffers_[0]);
    stream_func_(buffers_[1]);

    alSourceQueueBuffers(al_source_, 2, buffers_);
    alSourcePlay(al_source_);

    playing_sound_ = sound_id;
    loop_stream_ = loop;
}

void Source::update_source(float dt) {
    if(!playing_sound_) {
        return;
    }

    ALint processed = 0;

    alGetSourcei(al_source_, AL_BUFFERS_PROCESSED, &processed);

    while(processed--) {
        ALuint buffer = 0;
        alSourceUnqueueBuffers(al_source_, 1, &buffer);

        uint32_t bytes = stream_func_(buffer);

        if(!bytes) {
            signal_stream_finished_();

            if(loop_stream_) {
                play(playing_sound_, loop_stream_);
            } else {
                //Reset everything
                stream_func_ = std::function<int32_t (ALuint)>();
                playing_sound_ = SoundID();
            }
        }
        alSourceQueueBuffers(al_source_, 1, &buffer);
    }
}

bool Source::is_playing_sound() const {
    return bool(playing_sound_);
}

}
