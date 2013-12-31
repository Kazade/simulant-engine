#ifndef SOUND_H
#define SOUND_H

#include <vector>

#include <AL/al.h>
#include <AL/alc.h>

#include "generic/managed.h"
#include "generic/identifiable.h"

#include "kazbase/signals3/signals3.hpp"

#include "resource.h"
#include "loadable.h"

#include "types.h"

namespace kglt {

class Source;

class Sound :
    public Managed<Sound>,
    public generic::Identifiable<SoundID>,
    public Resource,
    public Loadable {

public:
    static void init_openal();
    static void shutdown_openal();

    Sound(ResourceManager* resource_manager, SoundID id);

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

    void set_source_init_function(std::function<void (Source&)> func) { init_source_ = func; }

private:
    std::function<void (Source&)> init_source_;

    std::vector<uint8_t> sound_data_;

    uint32_t sample_rate_;
    ALenum format_;
    uint8_t channels_;
    std::size_t buffer_size_;

    friend class Source;
};

typedef std::function<int32_t (ALuint)> StreamFunc;

class Source {

public:
    Source(Stage* stage);
    virtual ~Source();

    virtual void attach_sound(SoundID sound);

    void play_sound(bool loop=false);
    bool is_playing_sound() const;

    void update_source(float dt);

    sig::signal<void ()>& signal_stream_finished() { return signal_stream_finished_; }

    void set_stream_func(StreamFunc func) { stream_func_ = func; }

private:
    virtual bool can_attach_sound_by_id() const { return true; }

    Stage* stage_;

    ALuint al_source_;
    ALuint buffers_[2];

    StreamFunc stream_func_;
    SoundID sound_;

    bool playing_ = false;
    bool loop_stream_ = false;

    sig::signal<void ()> signal_stream_finished_;

    friend class Sound;
};

}
#endif // SOUND_H
