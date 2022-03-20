#pragma once

#include <cstdint>
#include <vector>
#include <memory>

#include "../generic/managed.h"
#include "../generic/range_value.h"

#include "../math/vec3.h"

namespace smlt {

class Sound;
class AudioSource;

typedef uint32_t AudioBufferID;
typedef uint32_t AudioSourceID;

typedef std::function<int32_t (AudioBufferID)> StreamFunc;

typedef std::size_t PlayingSoundID;

enum AudioRepeat {
    AUDIO_REPEAT_NONE,
    AUDIO_REPEAT_FOREVER
};

enum DistanceModel {
    DISTANCE_MODEL_POSITIONAL,
    DISTANCE_MODEL_AMBIENT,
    DISTANCE_MODEL_DEFAULT = DISTANCE_MODEL_POSITIONAL
};

class PlayingSound:
    public RefCounted<PlayingSound> {

    friend class AudioSource;

private:
    static PlayingSoundID counter_;

    PlayingSoundID id_;

    AudioSource& parent_;

    AudioSourceID source_;
    std::vector<AudioBufferID> buffers_;
    std::weak_ptr<Sound> sound_;
    StreamFunc stream_func_;

    AudioRepeat loop_stream_;
    bool is_dead_;

    /* This is used to calculate the velocity */
    smlt::Vec3 previous_position_;
    bool first_update_ = true;

    void start();
    void do_stop();

public:
    PlayingSound(AudioSource& parent, std::weak_ptr<Sound> sound, AudioRepeat loop_stream, DistanceModel model=DISTANCE_MODEL_POSITIONAL);
    virtual ~PlayingSound();

    PlayingSoundID id() const {
        return id_;
    }

    void update(float dt);
    void stop();

    bool is_playing() const;

    /* Set the stream function for filling buffers. A -1 return
     * means the sound has been destroyed */
    void set_stream_func(StreamFunc func) { stream_func_ = func; }

    bool is_dead() const { return is_dead_; }

    void set_gain(RangeValue<0, 1> gain);
    void set_pitch(RangeValue<0, 1> pitch);
    void set_reference_distance(float dist);
};

class PlayingSoundPtr {
    friend class AudioSource;

private:
    std::weak_ptr<PlayingSound> ptr_;

    PlayingSoundPtr(PlayingSound::ptr ptr):
        ptr_(ptr) {}

public:
    PlayingSoundPtr() = default;
    PlayingSoundPtr(const PlayingSoundPtr& rhs) = default;

    PlayingSoundPtr& operator=(const PlayingSoundPtr& rhs) {
        ptr_ = rhs.ptr_;
        return *this;
    }

    PlayingSound* operator->() const {
        auto lk = ptr_.lock();
        if(lk) {
            return lk.get();
        } else {
            return nullptr;
        }
    }

    bool is_valid() const {
        return bool(ptr_.lock());
    }

    operator bool() const {
        return is_valid();
    }
};

}
