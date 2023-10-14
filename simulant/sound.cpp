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
#include "time_keeper.h"
#include "threads/thread.h"

namespace smlt {



Sound::Sound(AssetID id, AssetManager *asset_manager, SoundDriver *sound_driver):
    generic::Identifiable<AssetID>(id),
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


}
