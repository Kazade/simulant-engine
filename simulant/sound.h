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
#include "sound/playing_sound.h"

#include "generic/managed.h"
#include "generic/identifiable.h"

#include "signals/signal.h"

#include "asset.h"
#include "loadable.h"

#include "types.h"

namespace smlt {

class StageNode;
class AudioSource;
class PlayingSound;
class Scene;

class Sound :
    public RefCounted<Sound>,
    public generic::Identifiable<AssetID>,
    public Asset,
    public Loadable,
    public ChainNameable<Sound> {

public:
    Sound(AssetID id, AssetManager* asset_manager, SoundDriver* sound_driver);

    uint32_t sample_rate() const { return sample_rate_; }
    void set_sample_rate(uint32_t rate) { sample_rate_ = rate; }

    AudioDataFormat format() { return format_; }
    void set_format(AudioDataFormat format) { format_ = format; }

    std::size_t buffer_size() const;

    uint8_t channels() const { return channels_; }
    void set_channels(uint8_t ch) { channels_ = ch; }

    std::shared_ptr<std::istream>& input_stream() { return sound_data_; }
    void set_input_stream(std::shared_ptr<std::istream> stream) {
        sound_data_ = stream;

        stream->seekg(0, std::ios_base::end);
        int end = stream->tellg();
        stream->seekg(0, std::ios_base::beg);
        stream_length_ = end;
    }

    std::size_t stream_length() const {
        return stream_length_;
    }

    template<typename Func>
    void set_playing_sound_init_function(Func&& func) {
        init_playing_sound_ = func;
    }

    SoundDriver* _driver() const { return driver_; }

private:
    void init_source(PlayingSound& source);

    std::function<void (PlayingSound&)> init_playing_sound_;

    SoundDriver* driver_ = nullptr;
    std::shared_ptr<std::istream> sound_data_;

    uint32_t sample_rate_ = 0;
    AudioDataFormat format_;
    uint8_t channels_ = 0;
    std::size_t stream_length_ = 0;

    friend class AudioSource;
    friend class PlayingSound;
};


typedef sig::signal<void (SoundPtr, AudioRepeat, DistanceModel)> SoundPlayedSignal;


class AudioSource {
    DEFINE_SIGNAL(SoundPlayedSignal, signal_sound_played);

public:
    AudioSource(Window* window);
    AudioSource(Scene* owner, StageNode* this_as_node, SoundDriver *driver);
    virtual ~AudioSource();

    PlayingSoundPtr play_sound(
        SoundPtr sound_id,
        AudioRepeat repeat=AUDIO_REPEAT_NONE,
        DistanceModel model=DISTANCE_MODEL_DEFAULT
    );

    bool stop_sound(PlayingAssetID sound_id);

    /* The number of sounds this source is currently playing */
    uint8_t playing_sound_count() const;

    /* The number of sounds that have finished, but aren't yet
     * destroyed */
    uint8_t played_sound_count() const;

    bool is_sound_playing() const;

    sig::signal<void ()>& signal_stream_finished() { return signal_stream_finished_; }

    void update_source(float dt);
protected:
    SoundDriver* _sound_driver() const;

public:
    Scene* scene_ = nullptr;
    Window* window_ = nullptr;
    SoundDriver* driver_ = nullptr;
    StageNode* node_ = nullptr;

    std::list<PlayingSound::ptr> instances_;
    sig::signal<void ()> signal_stream_finished_;

    friend class Sound;
    friend class PlayingSound;

    mutable thread::Mutex mutex_;
    static void source_update_thread();
};

}
#endif // SOUND_H
