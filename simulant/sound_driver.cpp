#include "application.h"
#include "window.h"

#include "sound_driver.h"
#include "sound.h"

namespace smlt {

SoundDriver::SoundDriver(Window *window):
    window_(window) {


}

SoundDriver::~SoundDriver() {
    source_update_.disconnect();
}

bool SoundDriver::startup() {
    return _startup();
}

void SoundDriver::shutdown() {
    _shutdown();
}

}
