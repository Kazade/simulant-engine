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
#include "application.h"

namespace smlt {

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
    /* If the source is destroyed we should stop all playing instances
     * immediately */
    auto app = smlt::get_app();
    SoundDriver* driver = (app) ? app->sound_driver.get() : nullptr;
    if(driver) {
        for(auto& instance: instances_) {
            driver->stop_source(instance->source_);
        }
    }
}

PlayingSoundPtr AudioSource::play_sound(SoundPtr sound, AudioRepeat repeat, DistanceModel model) {
    if(!sound) {
        S_WARN("Tried to play an invalid sound");
        return PlayingSoundPtr();
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

    return PlayingSoundPtr(new_source);
}

bool AudioSource::stop_sound(PlayingSoundID sound_id) {
    for(auto it = instances_.begin(); it != instances_.end();) {
        if((*it)->id() == sound_id) {
            (*it)->do_stop();
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

SoundDriver *AudioSource::_sound_driver() const {
    return get_app()->sound_driver.get();
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
