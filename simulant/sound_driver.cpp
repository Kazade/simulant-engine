#include "application.h"
#include "window.h"

#include "sound_driver.h"
#include "sound.h"

namespace smlt {

SoundDriver::SoundDriver(Window *window):
    window_(window),
    global_source_(nullptr) {


}

SoundDriver::~SoundDriver() {
    source_update_.disconnect();
    delete global_source_;
}

bool SoundDriver::startup() {
    global_source_ = new AudioSource(window_);

    return _startup();
}

void SoundDriver::shutdown() {
    delete global_source_;
    global_source_ = nullptr;
    _shutdown();
}

PlayingSoundPtr SoundDriver::play_sound(SoundPtr sound, AudioRepeat repeat) {
    return global_source_->play_sound(sound, repeat, DISTANCE_MODEL_AMBIENT);
}

}
