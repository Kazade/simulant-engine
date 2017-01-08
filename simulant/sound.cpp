//
//   Copyright (c) 2011-2017 Luke Benstead https://simulant-engine.appspot.com
//
//     This file is part of Simulant.
//
//     Simulant is free software: you can redistribute it and/or modify
//     it under the terms of the GNU General Public License as published by
//     the Free Software Foundation, either version 3 of the License, or
//     (at your option) any later version.
//
//     Simulant is distributed in the hope that it will be useful,
//     but WITHOUT ANY WARRANTY; without even the implied warranty of
//     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//     GNU General Public License for more details.
//
//     You should have received a copy of the GNU General Public License
//     along with Simulant.  If not, see <http://www.gnu.org/licenses/>.
//

#include "utils/al_error.h"
#include "window_base.h"
#include "stage.h"
#include "sound.h"

namespace smlt {

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

Sound::Sound(SoundID id, ResourceManager *resource_manager):
    generic::Identifiable<SoundID>(id),
    Resource(resource_manager) {


}

SourceInstance::SourceInstance(Source &parent, SoundID sound, bool loop_stream):
    parent_(parent),
    source_(0),
    buffers_{0, 0},
    sound_(sound),
    loop_stream_(loop_stream),
    is_dead_(false) {

    alGenSources(1, &source_);
    alGenBuffers(2, buffers_);
}

SourceInstance::~SourceInstance() {
    alDeleteSources(1, &source_);
    alDeleteBuffers(2, buffers_);
}

void SourceInstance::start() {
    //Fill up two buffers to begin with
    auto bs1 = stream_func_(buffers_[0]);
    auto bs2 = stream_func_(buffers_[1]);

    int to_queue = (bs1 && bs2) ? 2 : (bs1 || bs2)? 1 : 0;

    ALCheck(alSourceQueueBuffers, source_, to_queue, buffers_);
    ALCheck(alSourcePlay, source_);
}

bool SourceInstance::is_playing() const {
    ALint val;
    ALCheck(alGetSourcei, source_, AL_SOURCE_STATE, &val);
    return val == AL_PLAYING;
}

void SourceInstance::update(float dt) {
    ALint processed = 0;

    ALCheck(alGetSourcei, source_, AL_BUFFERS_PROCESSED, &processed);

    while(processed--) {
        ALuint buffer = 0;
        ALCheck(alSourceUnqueueBuffers, source_, 1, &buffer);
        uint32_t bytes = stream_func_(buffer);

        if(!bytes) {
            parent_.signal_stream_finished_();
            if(loop_stream_) {
                //Restart the sound
                auto sound = parent_.stage_->assets->sound(sound_);
                sound->init_source_(*this);
                start();
            } else {
                //Mark as dead
                is_dead_ = true;
            }
        } else {
            ALCheck(alSourceQueueBuffers, source_, 1, &buffer);
        }
    }
}

Source::Source(WindowBase *window):
    stage_(nullptr),
    window_(window) {

}

Source::Source(Stage *stage):
    stage_(stage),
    window_(nullptr) {


}

Source::~Source() {

}

void Source::play_sound(SoundID sound, bool loop) {
    if(!sound) {
        L_WARN("Tried to play an invalid sound");
        return;
    }

    SourceInstance::ptr new_source = SourceInstance::create(*this, sound, loop);

    /* This is surely wrong... */
    if(stage_) {
        auto s = stage_->assets->sound(sound);
        s->init_source_(*new_source);
    } else {
        auto s = window_->shared_assets->sound(sound);
        s->init_source_(*new_source);
    }

    new_source->start();

    instances_.push_back(new_source);
}

void Source::update_source(float dt) {
    for(auto instance: instances_) {
        instance->update(dt);
    }

    //Remove any instances that have finished playing
    instances_.erase(
        std::remove_if(
            instances_.begin(),
            instances_.end(),
            std::bind(&SourceInstance::is_dead, std::placeholders::_1)
        ),
        instances_.end()
    );
}

int32_t Source::playing_sound_count() const {
    int32_t i = 0;
    for(auto instance: instances_) {
        if(instance->is_playing()) {
            i++;
        }
    }
    return i;
}

}
