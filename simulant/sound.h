/* *   Copyright (c) 2011-2017 Luke Benstead https://simulant-engine.appspot.com
 *
 *     This file is part of Simulant.
 *
 *     Simulant is free software: you can redistribute it and/or modify
 *     it under the terms of the GNU Lesser General Public License as published by
 *     the Free Software Foundation, either version 3 of the License, or
 *     (at your option) any later version.
 *
 *     Simulant is distributed in the hope that it will be useful,
 *     but WITHOUT ANY WARRANTY; without even the implied warranty of
 *     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *     GNU Lesser General Public License for more details.
 *
 *     You should have received a copy of the GNU Lesser General Public License
 *     along with Simulant.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef SOUND_H
#define SOUND_H

#include <vector>
#include <list>

#include "sound_driver.h"

#include "generic/managed.h"
#include "generic/identifiable.h"

#include "deps/kazsignal/kazsignal.h"

#include "asset.h"
#include "loadable.h"

#include "types.h"

namespace smlt {

class StageNode;
class Source;
class SourceInstance;

class Sound :
    public RefCounted<Sound>,
    public generic::Identifiable<SoundID>,
    public Asset,
    public Loadable {

public:
    Sound(SoundID id, AssetManager* asset_manager, SoundDriver* sound_driver);

    uint32_t sample_rate() const { return sample_rate_; }
    void set_sample_rate(uint32_t rate) { sample_rate_ = rate; }

    AudioDataFormat format() { return format_; }
    void set_format(AudioDataFormat format) { format_ = format; }

    std::size_t buffer_size() const { return buffer_size_; }
    void set_buffer_size(std::size_t size) { buffer_size_ = size; }

    uint8_t channels() const { return channels_; }
    void set_channels(uint8_t ch) { channels_ = ch; }

    std::vector<uint8_t>& data() { return sound_data_; }
    void set_data(const std::vector<uint8_t>& data) { sound_data_ = data; }

    void set_source_init_function(std::function<void (SourceInstance&)> func) { init_source_ = func; }

    SoundDriver* _driver() const { return driver_; }
private:
    void init_source(SourceInstance& source);

    std::function<void (SourceInstance&)> init_source_;

    SoundDriver* driver_ = nullptr;
    std::vector<uint8_t> sound_data_;

    uint32_t sample_rate_ = 0;
    AudioDataFormat format_;
    uint8_t channels_ = 0;
    std::size_t buffer_size_ = 0;

    friend class Source;
    friend class SourceInstance;
};

typedef std::function<int32_t (AudioBufferID)> StreamFunc;

class Source;

enum AudioRepeat {
    AUDIO_REPEAT_NONE,
    AUDIO_REPEAT_FOREVER
};

enum DistanceModel {
    DISTANCE_MODEL_POSITIONAL,
    DISTANCE_MODEL_AMBIENT
};

class SourceInstance:
    public RefCounted<SourceInstance> {

    friend class Source;
private:
    Source& parent_;

    AudioSourceID source_;
    std::vector<AudioBufferID> buffers_;
    SoundID sound_;
    StreamFunc stream_func_;

    AudioRepeat loop_stream_;
    bool is_dead_;

    /* This is used to calculate the velocity */
    smlt::Vec3 previous_position_;
    bool first_update_ = true;
public:
    SourceInstance(Source& parent, SoundID sound, AudioRepeat loop_stream, DistanceModel model=DISTANCE_MODEL_POSITIONAL);
    ~SourceInstance();

    void start();
    void update(float dt);

    bool is_playing() const;
    void set_stream_func(StreamFunc func) { stream_func_ = func; }

    bool is_dead() const { return is_dead_; }
};

class Source {
public:
    Source(Window* window);
    Source(Stage* stage, SoundDriver *driver);
    virtual ~Source();

    void play_sound(SoundID sound, AudioRepeat repeat=AUDIO_REPEAT_NONE);
    int32_t playing_sound_count() const;

    void update_source(float dt);

    sig::signal<void ()>& signal_stream_finished() { return signal_stream_finished_; }

    void set_gain(RangeValue<0, 1> gain);
    void set_pitch(RangeValue<0, 1> pitch);
    void set_reference_distance(float dist);

private:
    SoundDriver* _sound_driver() const;

    Stage* stage_ = nullptr;
    Window* window_ = nullptr;
    SoundDriver* driver_ = nullptr;
    StageNode* node_ = nullptr;

    std::list<SourceInstance::ptr> instances_;
    sig::signal<void ()> signal_stream_finished_;

    friend class Sound;
    friend class SourceInstance;
};

}
#endif // SOUND_H
