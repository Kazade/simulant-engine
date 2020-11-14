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

SourceInstanceID SourceInstance::counter_ = 0;

Sound::Sound(SoundID id, AssetManager *asset_manager, SoundDriver *sound_driver):
    generic::Identifiable<SoundID>(id),
    Asset(asset_manager),
    driver_(sound_driver) {

}

void Sound::init_source(SourceInstance& source) {
    if(!init_source_) return; // Nothing to do

    init_source_(source);
}

SourceInstance::SourceInstance(Source &parent, std::weak_ptr<Sound> sound, AudioRepeat loop_stream, DistanceModel model):
    id_(++SourceInstance::counter_),
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

SourceInstance::~SourceInstance() {
    SoundDriver* driver = parent_._sound_driver();

    if(driver) {
        driver->stop_source(source_); // Make sure we have stopped playing!
        driver->destroy_sources({source_});
        driver->destroy_buffers(buffers_);
    }
}

void SourceInstance::start() {
    if(!stream_func_) {
        L_WARN("Not playing sound as no stream func was set");
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

void SourceInstance::stop() {
    SoundDriver* driver = parent_._sound_driver();

    driver->stop_source(source_);
    is_dead_ = true;
}

bool SourceInstance::is_playing() const {
    SoundDriver* driver = parent_._sound_driver();
    return driver->source_state(source_) == AUDIO_SOURCE_STATE_PLAYING;
}

void SourceInstance::update(float dt) {
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

    while(processed--) {
        AudioBufferID buffer = driver->unqueue_buffers_from_source(source_, 1).back();

        int32_t bytes = stream_func_(buffer);

        if(bytes < 0) {
            /* -1 indicates the sound has been deleted */
            is_dead_ = true;
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
            auto sound = sound_.lock();
            if(sound) {
                sound->init_source(*this);
                start();
            } else {
                is_dead_ = true;
            }
        } else {
            //Mark as dead
            is_dead_ = true;
        }
    }
}

Source::Source(Window *window):
    stage_(nullptr),
    window_(window) {
}

Source::Source(Stage *stage, StageNode* this_as_node, SoundDriver* driver):
    stage_(stage),
    window_(nullptr),
    driver_(driver),
    node_(this_as_node) {

}

Source::~Source() {

}

SourceInstanceID Source::play_sound(SoundID sound_id, AudioRepeat repeat) {
    if(!sound_id) {
        L_WARN("Tried to play an invalid sound");
        return 0;
    }

    /* There's surely a better way of determining which asset manager to use here */
    auto asset_manager = (stage_) ? stage_->assets.get() : window_->shared_assets.get();
    auto sound = asset_manager->sound(sound_id);

    assert(sound);

    // If this is the window, we create an ambient source
    SourceInstance::ptr new_source = SourceInstance::create(
        *this,
        sound,
        repeat,
        (stage_) ? DISTANCE_MODEL_POSITIONAL : DISTANCE_MODEL_AMBIENT
    );

    sound->init_source(*new_source);
    new_source->start();

    instances_.push_back(new_source);

    return new_source->id();
}

bool Source::stop_sound(SourceInstanceID sound_id) {
    for(auto& instance: instances_) {
        if(instance->id() == sound_id) {
            instance->stop();
            return true;
        }
    }

    return false;
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

void Source::set_pitch(RangeValue<0, 1> pitch) {
    for(auto& instance: instances_) {
        _sound_driver()->set_source_pitch(instance->source_, pitch);
    }
}

void Source::set_reference_distance(float dist) {
    for(auto& instance: instances_) {
        _sound_driver()->set_source_reference_distance(instance->source_, dist);
    }
}

void Source::set_gain(RangeValue<0, 1> gain) {
    for(auto& instance: instances_) {
        _sound_driver()->set_source_gain(instance->source_, gain);
    }
}

SoundDriver *Source::_sound_driver() const {
    return (window_) ? window_->_sound_driver() : driver_;
}

uint8_t Source::playing_sound_count() const {
    uint8_t i = 0;
    for(auto instance: instances_) {
        if(instance->is_playing()) {
            i++;
        }
    }
    return i;
}

uint8_t Source::played_sound_count() const {
    return std::count_if(
        instances_.begin(),
        instances_.end(),
        [](SourceInstance::ptr ptr) -> bool {
            return ptr->is_dead();
        }
    );
}

bool Source::is_sound_playing() const {
    return playing_sound_count() > 0;
}

}
