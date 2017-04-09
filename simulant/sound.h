/* *   Copyright (c) 2011-2017 Luke Benstead https://simulant-engine.appspot.com
 *
 *     This file is part of Simulant.
 *
 *     Simulant is free software: you can redistribute it and/or modify
 *     it under the terms of the GNU General Public License as published by
 *     the Free Software Foundation, either version 3 of the License, or
 *     (at your option) any later version.
 *
 *     Simulant is distributed in the hope that it will be useful,
 *     but WITHOUT ANY WARRANTY; without even the implied warranty of
 *     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *     GNU General Public License for more details.
 *
 *     You should have received a copy of the GNU General Public License
 *     along with Simulant.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef SOUND_H
#define SOUND_H

#include <vector>
#include <list>

#ifdef __APPLE__
#include <OpenAL/al.h>
#include <OpenAL/alc.h>
#else
#include <AL/al.h>
#include <AL/alc.h>
#endif

#include "generic/managed.h"
#include "generic/identifiable.h"

#include "deps/kazsignal/kazsignal.h"

#include "resource.h"
#include "loadable.h"

#include "types.h"

namespace smlt {

class Source;
class SourceInstance;

class Sound :
    public Managed<Sound>,
    public generic::Identifiable<SoundID>,
    public Resource,
    public Loadable,
    public std::enable_shared_from_this<Sound> {

public:
    static void init_openal();
    static void shutdown_openal();

    Sound(SoundID id, ResourceManager* resource_manager);

    uint32_t sample_rate() const { return sample_rate_; }
    void set_sample_rate(uint32_t rate) { sample_rate_ = rate; }

    ALenum format() { return format_; }
    void set_format(ALenum format) { format_ = format; }

    std::size_t buffer_size() const { return buffer_size_; }
    void set_buffer_size(std::size_t size) { buffer_size_ = size; }

    uint8_t channels() const { return channels_; }
    void set_channels(uint8_t ch) { channels_ = ch; }

    std::vector<uint8_t>& data() { return sound_data_; }
    void set_data(const std::vector<uint8_t>& data) { sound_data_ = data; }

    void set_source_init_function(std::function<void (SourceInstance&)> func) { init_source_ = func; }

    static bool is_disabled();
private:
    std::function<void (SourceInstance&)> init_source_;

    std::vector<uint8_t> sound_data_;

    uint32_t sample_rate_ = 0;
    ALenum format_;
    uint8_t channels_ = 0;
    std::size_t buffer_size_ = 0;

    friend class Source;
    friend class SourceInstance;
};

typedef std::function<int32_t (ALuint)> StreamFunc;

class Source;

class SourceInstance:
    public Managed<SourceInstance> {

private:
    Source& parent_;

    ALuint source_;
    ALuint buffers_[2];
    SoundID sound_;
    StreamFunc stream_func_;

    bool loop_stream_;
    bool is_dead_;

public:
    SourceInstance(Source& parent, SoundID sound, bool loop_stream);
    ~SourceInstance();

    void start();
    void update(float dt);

    bool is_playing() const;
    void set_stream_func(StreamFunc func) { stream_func_ = func; }

    bool is_dead() const { return is_dead_; }
};

class Source {
public:
    Source(WindowBase* window);
    Source(Stage* stage);
    virtual ~Source();

    void play_sound(SoundID sound, bool loop=false);
    int32_t playing_sound_count() const;

    void update_source(float dt);

    sig::signal<void ()>& signal_stream_finished() { return signal_stream_finished_; }

private:
    Stage* stage_;
    WindowBase* window_;

    std::list<SourceInstance::ptr> instances_;
    sig::signal<void ()> signal_stream_finished_;

    friend class Sound;
    friend class SourceInstance;
};

}
#endif // SOUND_H
