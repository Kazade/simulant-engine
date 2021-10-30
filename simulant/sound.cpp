//
//   Copyright (c) 2011-2017 Luke Benstead https://simulant-engine.appspot.com
//
//     This file is part of Simulant.
//
//     Simulant is free software: you can redistribute it and/or modify
//     it under the terms of the GNU Lesser General Public License as published by
//     the Free Software Foundation, either version 3 of the License, or
//     (at your option) any later version.
//
//     Simulant is distributed in the hope that it will be useful,
//     but WITHOUT ANY WARRANTY; without even the implied warranty of
//     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//     GNU Lesser General Public License for more details.
//
//     You should have received a copy of the GNU Lesser General Public License
//     along with Simulant.  If not, see <http://www.gnu.org/licenses/>.
//

#include "logging.h"
#include "window.h"
#include "stage.h"
#include "sound.h"
#include "sound_driver.h"
#include "nodes/stage_node.h"

namespace smlt {

PlayingSoundID PlayingSound::counter_ = 0;

Sound::Sound(SoundID id, AssetManager *asset_manager, SoundDriver *sound_driver):
    generic::Identifiable<SoundID>(id),
    Asset(asset_manager),
    driver_(sound_driver) {

}

std::size_t Sound::buffer_size() const {
    /* We try to determine the optimum buffer size depending on the
     * frequency, number of channels and format. Testing shows that you need
     * at least 0.5 seconds of data to minimize the risk of being unable
     * to queue data in time. Always returns a power of two amount. */
    const float DURATION = 0.5f;

    const std::size_t bytes_per_sample = audio_data_format_byte_size(format_);
    const std::size_t data_size_per_second = bytes_per_sample * sample_rate_;
    const std::size_t required_size = data_size_per_second * DURATION;

    std::size_t ret = next_power_of_two(required_size);

#ifdef __DREAMCAST__
    /* The Dreamcast sound chip only allows 65534 samples and ALdc
     * will truncate if it's larger so this prevents us triggering
     * an OpenAL error */
    ret = std::min(ret, (std::size_t) 65534);
#endif

    return ret;
}

void Sound::init_source(PlayingSound& source) {
    if(!init_playing_sound_) return; // Nothing to do

    init_playing_sound_(source);
}

PlayingSound::PlayingSound(AudioSource &parent, std::weak_ptr<Sound> sound, AudioRepeat loop_stream, DistanceModel model):
    id_(++PlayingSound::counter_),
    parent_(parent),
    source_(0),
    buffers_{0, 0},
    sound_(sound),
    loop_stream_(loop_stream),
    is_dead_(false) {


    SoundDriver* driver = parent_._sound_driver();

    source_ = driver->generate_sources(1).back();
    buffers_ = driver->generate_buffers(2);

    if(model == DISTANCE_MODEL_AMBIENT) {
        driver->set_source_as_ambient(source_);
    }
}

PlayingSound::~PlayingSound() {
    SoundDriver* driver = parent_._sound_driver();

    if(driver) {
        driver->stop_source(source_); // Make sure we have stopped playing!
        driver->destroy_sources({source_});
        driver->destroy_buffers(buffers_);
    }
}

void PlayingSound::start() {
    if(!stream_func_) {
        S_WARN("Not playing sound as no stream func was set");
        return;
    }

    //Fill up two buffers to begin with
    auto bs1 = stream_func_(buffers_[0]);
    auto bs2 = stream_func_(buffers_[1]);

    if(bs1 < 0 || bs2 < 0) {
        /* Sound was destroyed immediately */
        is_dead_ = true;
        return;
    }

    int to_queue = (bs1 && bs2) ? 2 : (bs1 || bs2)? 1 : 0;

    SoundDriver* driver = parent_._sound_driver();

    driver->queue_buffers_to_source(source_, to_queue, buffers_);
    driver->play_source(source_);
}

void PlayingSound::stop() {
    SoundDriver* driver = parent_._sound_driver();

    driver->stop_source(source_);
    is_dead_ = true;
}

bool PlayingSound::is_playing() const {
    SoundDriver* driver = parent_._sound_driver();
    return driver->source_state(source_) == AUDIO_SOURCE_STATE_PLAYING;
}

void PlayingSound::update(float dt) {
    SoundDriver* driver = parent_._sound_driver();

    // Update the position of the source if this is attached to a stagenode
    if(parent_.node_) {
        auto pos = parent_.node_->absolute_position();
        driver->set_source_properties(
            source_,
            pos,
            // Use the last position to calculate the velocity, this is a bit of
            // a hack... FIXME maybe..
            // FIXME: This is value is "scaled" to assume the velocity over a second
            // this isn't accurate as update isn't called with a fixed timestep
            (first_update_) ? smlt::Vec3() : (pos - previous_position_) * (1.0f / dt)
        );

        previous_position_ = pos;

        // We don't apply velocity on the first update otherwise things might go
        // funny
        first_update_ = false;
    }

    int32_t processed = driver->source_buffers_processed_count(source_);

    bool finished = false;

    /* We lock through the entire update, mainly so that if a sound finishes
     * but it's looping, we don't lose the sound before we reinitialise the
     * source instance */
    auto sound = sound_.lock();

    while(processed--) {
        AudioBufferID buffer = driver->unqueue_buffers_from_source(source_, 1).back();

        int32_t bytes = stream_func_(buffer);

        if(bytes <= 0) {
            /* -1 indicates the sound has been deleted */
            finished = true;
            break;
        }

        if(!finished) {
            if(!bytes) {
                // Just because we have nothing left to queue, doesn't mean that all buffers
                // are finished, so wait for the last buffer to be unqueued
                finished = driver->source_state(source_) == AUDIO_SOURCE_STATE_STOPPED;
            } else {
                driver->queue_buffers_to_source(source_, 1, {buffer});
            }
        }
    }

    if(finished) {
        parent_.signal_stream_finished_();

        /* Make sure we're totally stopped! */
        driver->stop_source(source_);

        /* Make totally sure we've unqueued everything */
        processed = driver->source_buffers_processed_count(source_);
        while(processed--) {
            driver->unqueue_buffers_from_source(source_, 1).back();
        }

        if(loop_stream_ == AUDIO_REPEAT_FOREVER) {
            //Restart the sound
            if(sound) {
                sound->init_source(*this);
                start();
                is_dead_ = false;
            } else {
                S_WARN("Sound unexpectedly vanished while looping");
                is_dead_ = true;
            }
        } else {
            //Mark as dead
            is_dead_ = true;
        }
    }
}

AudioSource::AudioSource(Window *window):
    stage_(nullptr),
    window_(window) {
}

AudioSource::AudioSource(Stage *stage, StageNode* this_as_node, SoundDriver* driver):
    stage_(stage),
    window_(nullptr),
    driver_(driver),
    node_(this_as_node) {

}

AudioSource::~AudioSource() {

}

PlayingSoundID AudioSource::play_sound(SoundPtr sound, AudioRepeat repeat, DistanceModel model) {
    if(!sound) {
        S_WARN("Tried to play an invalid sound");
        return 0;
    }

    assert(sound);

    // If this is the window, we create an ambient source
    PlayingSound::ptr new_source = PlayingSound::create(
        *this,
        sound,
        repeat,
        model
    );

    sound->init_source(*new_source);
    new_source->start();

    instances_.push_back(new_source);

    return new_source->id();
}

bool AudioSource::stop_sound(PlayingSoundID sound_id) {
    for(auto it = instances_.begin(); it != instances_.end();) {
        if((*it)->id() == sound_id) {
            (*it)->stop();
            it = instances_.erase(it);
            return true;
        } else {
            ++it;
        }
    }

    return false;
}

void AudioSource::update_source(float dt) {
    //Remove any instances that have finished playing
    instances_.erase(
        std::remove_if(
            instances_.begin(),
            instances_.end(),
            std::bind(&PlayingSound::is_dead, std::placeholders::_1)
        ),
        instances_.end()
    );

    for(auto instance: instances_) {
        instance->update(dt);
    }
}

void AudioSource::set_pitch(RangeValue<0, 1> pitch) {
    for(auto& instance: instances_) {
        _sound_driver()->set_source_pitch(instance->source_, pitch);
    }
}

void AudioSource::set_reference_distance(float dist) {
    for(auto& instance: instances_) {
        _sound_driver()->set_source_reference_distance(instance->source_, dist);
    }
}

void AudioSource::set_gain(RangeValue<0, 1> gain) {
    for(auto& instance: instances_) {
        _sound_driver()->set_source_gain(instance->source_, gain);
    }
}

SoundDriver *AudioSource::_sound_driver() const {
    return (window_) ? window_->_sound_driver() : driver_;
}

uint8_t AudioSource::playing_sound_count() const {
    uint8_t i = 0;
    for(auto instance: instances_) {
        if(instance->is_playing()) {
            i++;
        }
    }
    return i;
}

uint8_t AudioSource::played_sound_count() const {
    return std::count_if(
        instances_.begin(),
        instances_.end(),
        [](PlayingSound::ptr ptr) -> bool {
            return ptr->is_dead();
        }
    );
}

bool AudioSource::is_sound_playing() const {
    return playing_sound_count() > 0;
}

}
