#include "application.h"
#include "window.h"

#include "sound_driver.h"
#include "sound.h"

namespace smlt {

SoundDriver::SoundDriver(Window *window):
    window_(window),
    global_source_(new AudioSource(window)) {

    source_update_ = window_->application->signal_update().connect([&](float dt) {
        global_source_->update_source(dt);
    });
}

SoundDriver::~SoundDriver() {
    source_update_.disconnect();
    delete global_source_;
}

PlayingSoundPtr SoundDriver::play_sound(SoundPtr sound, AudioRepeat repeat) {
    return global_source_->play_sound(sound, repeat, DISTANCE_MODEL_AMBIENT);
}

}
