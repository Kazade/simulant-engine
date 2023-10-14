#pragma once

#include "stage_node.h"

namespace smlt {

typedef sig::signal<void (SoundPtr, AudioRepeat, DistanceModel)> SoundPlayedSignal;
typedef sig::signal<void ()> StreamFinishedSignal;

struct AudioSourceParams {};

class AudioSource : public StageNode {

    DEFINE_SIGNAL(SoundPlayedSignal, signal_sound_played);
    DEFINE_SIGNAL(StreamFinishedSignal, signal_stream_finished);

public:

    struct Meta {
        const static StageNodeType node_type = STAGE_NODE_TYPE_AUDIO_SOURCE;
        typedef AudioSourceParams params_type;
    };

    AudioSource(Scene* owner):
        StageNode(owner, STAGE_NODE_TYPE_AUDIO_SOURCE) {}

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

    void update_source(float dt);

    const AABB& aabb() const {
        return AABB::ZERO;
    }

protected:
    SoundDriver* _sound_driver() const;

public:
    bool on_create(void* params) override;
    bool on_destroy() override;

    Scene* scene_ = nullptr;
    SoundDriver* driver_ = nullptr;

    std::list<PlayingSound::ptr> instances_;

    friend class Sound;
    friend class PlayingSound;

    mutable thread::Mutex mutex_;
    static void source_update_thread();
};

}
