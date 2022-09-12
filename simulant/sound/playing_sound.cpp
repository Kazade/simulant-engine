#include "playing_sound.h"
#include "../sound_driver.h"
#include "../sound.h"
#include "../application.h"
#include "../nodes/stage_node.h"

namespace smlt {

const static int BUFFER_COUNT = 4;

PlayingSoundID PlayingSound::counter_ = 0;

PlayingSound::PlayingSound(AudioSource &parent, std::weak_ptr<Sound> sound, AudioRepeat loop_stream, DistanceModel model):
    id_(++PlayingSound::counter_),
    parent_(parent),
    source_(0),
    buffers_{0, 0, 0, 0},
    sound_(sound),
    loop_stream_(loop_stream),
    is_dead_(false) {

    SoundDriver* driver = smlt::get_app()->sound_driver.get();

    source_ = driver->generate_sources(1).back();
    buffers_ = driver->generate_buffers(BUFFER_COUNT);

    if(model == DISTANCE_MODEL_AMBIENT) {
        driver->set_source_as_ambient(source_);
    }
}

PlayingSound::~PlayingSound() {
    SoundDriver* driver = smlt::get_app()->sound_driver.get();

    if(driver) {
        driver->stop_source(source_); // Make sure we have stopped playing!
        driver->destroy_sources({source_});
        driver->destroy_buffers(buffers_);
    }
}

void PlayingSound::start() {
    if(!stream_func_) {
        S_WARN("Not playing sound as no stream func was set");
        return;
    }

    SoundDriver* driver = smlt::get_app()->sound_driver.get();

    for(int i = 0; i < BUFFER_COUNT; ++i) {
        auto bs = stream_func_(buffers_[i]);
        if(bs < 0) {
            /* Sound was destroyed immediately */
            is_dead_ = true;
            return;
        } else if(!bs) {
            /* We don't need any more buffers */
            break;
        } else {
            /* We queue each buffer one after the other, and then play once the first
             * buffer is pushed. This avoids a stall while all initial buffers are uploaded */
            std::vector<AudioBufferID> t = {buffers_[i]};
            driver->queue_buffers_to_source(source_, 1, t);

            if(i == 0) {
                driver->play_source(source_);
            }
        }
    }
}

void PlayingSound::do_stop() {
    auto app = smlt::get_app();
    SoundDriver* driver = (app) ? app->sound_driver.get() : nullptr;

    if(driver) {
        driver->stop_source(source_);
    }
    is_dead_ = true;
}

bool PlayingSound::is_playing() const {
    SoundDriver* driver = smlt::get_app()->sound_driver.get();
    return driver->source_state(source_) == AUDIO_SOURCE_STATE_PLAYING;
}

void PlayingSound::set_gain(RangeValue<0, 1> gain) {
    auto driver = get_app()->sound_driver.get();
    driver->set_source_gain(source_, gain);
}

void PlayingSound::set_pitch(RangeValue<0, 1> pitch) {
    auto driver = get_app()->sound_driver.get();
    driver->set_source_pitch(source_, pitch);
}

void PlayingSound::set_reference_distance(float dist) {
    auto driver = get_app()->sound_driver.get();
    driver->set_source_reference_distance(source_, dist);
}

void PlayingSound::update(float dt) {
    SoundDriver* driver = smlt::get_app()->sound_driver.get();

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

    /* We lock through the entire update, mainly so that if a sound finishes
     * but it's looping, we don't lose the sound before we reinitialise the
     * source instance */
    auto sound = sound_.lock();    

    while(processed--) {
        AudioBufferID buffer = driver->unqueue_buffers_from_source(source_, 1).front();

        int32_t bytes = stream_func_(buffer);

        if(!finished) {
            if(bytes <= 0) {
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
        driver->unqueue_buffers_from_source(source_, processed);

        if(loop_stream_ == AUDIO_REPEAT_FOREVER) {
            //Restart the sound
            if(sound) {
                sound->init_source(*this);
                start();
                is_dead_ = false;
            } else {
                S_WARN("Sound unexpectedly vanished while looping");
                is_dead_ = true;
            }
        } else {
            //Mark as dead
            is_dead_ = true;
        }
    }
}

void PlayingSound::stop() {
    /* We don't call do_stop directly because the source
     * will need to remove the playing sound from its list */
    parent_.stop_sound(id());
}

}
