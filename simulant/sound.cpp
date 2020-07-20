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
#include "core.h"
#include "stage.h"
#include "sound.h"
#include "sound_driver.h"
#include "nodes/stage_node.h"

namespace smlt {

Sound::Sound(SoundID id, AssetManager *asset_manager, SoundDriver *sound_driver):
    generic::Identifiable<SoundID>(id),
    Asset(asset_manager),
    driver_(sound_driver) {

}

void Sound::init_source(SourceInstance& source) {
    if(!init_source_) return; // Nothing to do

    init_source_(source);
}

SourceInstance::SourceInstance(Source &parent, SoundID sound, AudioRepeat loop_stream, DistanceModel model):
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

    int to_queue = (bs1 && bs2) ? 2 : (bs1 || bs2)? 1 : 0;

    SoundDriver* driver = parent_._sound_driver();

    driver->queue_buffers_to_source(source_, to_queue, buffers_);
    driver->play_source(source_);
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
            parent_.node_->absolute_rotation(),
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

    while(processed--) {
        AudioBufferID buffer = driver->unqueue_buffers_from_source(source_, 1).back();

        uint32_t bytes = stream_func_(buffer);

        if(!bytes) {
            // Just because we have nothing left to queue, doesn't mean that all buffers
            // are finished, so wait for the last buffer to be unqueued
            bool finished = driver->source_state(source_) == AUDIO_SOURCE_STATE_STOPPED;
            if(finished) {
                parent_.signal_stream_finished_();
                if(loop_stream_ == AUDIO_REPEAT_FOREVER) {
                    //Restart the sound
                    auto sound = parent_.stage_->assets->sound(sound_);
                    assert(sound);
                    sound->init_source(*this);
                    start();
                } else {
                    //Mark as dead
                    is_dead_ = true;
                }
            }

        } else {
            driver->queue_buffers_to_source(source_, 1, {buffer});
        }
    }
}

Source::Source(Core *core):
    stage_(nullptr),
    core_(core) {
}

Source::Source(Stage *stage, SoundDriver* driver):
    stage_(stage),
    core_(nullptr),
    driver_(driver) {

    // Store whether or not this source is a stagenode
    // if it is, we'll update the position etc.
    node_ = dynamic_cast<StageNode*>(this);
}

Source::~Source() {

}

void Source::play_sound(SoundID sound, AudioRepeat repeat) {
    if(!sound) {
        L_WARN("Tried to play an invalid sound");
        return;
    }

    // If this is the window, we create an ambient source
    SourceInstance::ptr new_source = SourceInstance::create(
        *this,
        sound,
        repeat,
        (stage_) ? DISTANCE_MODEL_POSITIONAL : DISTANCE_MODEL_AMBIENT
    );

    /* There's surely a better way of determining which asset manager to use here */
    if(stage_) {
        auto s = stage_->assets->sound(sound);
        s->init_source(*new_source);
    } else {
        auto s = core_->shared_assets->sound(sound);
        s->init_source(*new_source);
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
    return (core_) ? core_->_sound_driver() : driver_;
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
