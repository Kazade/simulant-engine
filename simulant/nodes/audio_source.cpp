
#include "audio_source.h"
#include "../application.h"
#include "../time_keeper.h"

namespace smlt {

static thread::Mutex ACTIVE_SOURCES_MUTEX;
static std::list<AudioSource*> ACTIVE_SOURCES;
static std::shared_ptr<thread::Thread> SOURCE_UPDATE_THREAD;

AudioSource::~AudioSource() {
    assert(is_destroyed());
}

bool AudioSource::on_create(void* params) {
    _S_UNUSED(params);

    thread::Lock<thread::Mutex> glock(ACTIVE_SOURCES_MUTEX);

    /* Start the source update thread if we didn't already */
    if(!SOURCE_UPDATE_THREAD) {
        SOURCE_UPDATE_THREAD = std::make_shared<thread::Thread>(&source_update_thread);

        /* When the app shuts down, wait for the thread to finish before continuing
         * with the shutdown process */
        get_app()->signal_shutdown().connect([&]() {
            SOURCE_UPDATE_THREAD->join();
            SOURCE_UPDATE_THREAD.reset();
        });
    }

    ACTIVE_SOURCES.push_back(this);

    auto app = get_app();
    driver_ = (app) ? app->sound_driver.get() : nullptr;

    return true;
}

bool AudioSource::on_destroy() {
    /* If the source is destroyed we should stop all playing instances
     * immediately */
    thread::Lock<thread::Mutex> glock(ACTIVE_SOURCES_MUTEX);
    ACTIVE_SOURCES.remove(this);

    thread::Lock<thread::Mutex> lock(mutex_);
    if(driver_) {
        for(auto& instance: instances_) {
            driver_->stop_source(instance->source_);
        }
    }

    return true;
}

void AudioSource::source_update_thread() {
    static auto update_rate = 1.0f / 20.0f;
    static auto last_time = get_app()->time_keeper->now_in_us();

    S_INFO("Starting source update thread");

    while(true) {
        if(get_app() && get_app()->is_shutting_down()) {
            break;
        }

        auto now = get_app()->time_keeper->now_in_us();
        auto diff = now - last_time;
        auto dt = float(diff) * 0.000001f;

        if(dt < update_rate) {
            thread::yield();
            continue;
        }

        {
            thread::Lock<thread::Mutex> glock(ACTIVE_SOURCES_MUTEX);
            for(auto src: ACTIVE_SOURCES) {
                src->update_source(dt * get_app()->time_keeper->time_scale());
            }
        }

        last_time = now;
    }

    S_INFO("Stopping audio thread");
}

PlayingSoundPtr AudioSource::play_sound(SoundPtr sound, AudioRepeat repeat, DistanceModel model) {
    if(!sound) {
        S_WARN("Tried to play an invalid sound");
        return PlayingSoundPtr();
    }

    assert(sound);

    thread::Lock<thread::Mutex> lock(mutex_);

    // If this is the window, we create an ambient source
    PlayingSound::ptr new_source = PlayingSound::create(
        this,
        sound,
        repeat,
        model
    );

    sound->init_source(*new_source);
    new_source->start();

    instances_.push_back(new_source);

    signal_sound_played_(sound, repeat, model);

    return PlayingSoundPtr(new_source);
}

bool AudioSource::stop_sound(PlayingAssetID sound_id) {
    thread::Lock<thread::Mutex> lock(mutex_);

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
    thread::Lock<thread::Mutex> lock(mutex_);

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
    return driver_;
}

uint8_t AudioSource::playing_sound_count() const {
    thread::Lock<thread::Mutex> lock(mutex_);

    uint8_t i = 0;
    for(auto instance: instances_) {
        if(instance->is_playing()) {
            i++;
        }
    }
    return i;
}

uint8_t AudioSource::played_sound_count() const {
    thread::Lock<thread::Mutex> lock(mutex_);

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
