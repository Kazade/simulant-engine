#include <memory>
#include "kos_window.h"
#include "sound_drivers/kos_sound_driver.h"

namespace smlt {

KOSWindow::KOSWindow(uint32_t width, uint32_t height, uint32_t bpp, bool fullscreen) {
    set_width(width);
    set_height(height);
    set_bpp(bpp);
    set_fullscreen(fullscreen);
}


void KOSWindow::swap_buffers() {

}

bool KOSWindow::create_window(int width, int height, int bpp, bool fullscreen) {

    return true;
}

void KOSWindow::destroy_window() {

}

void KOSWindow::check_events() {

}

std::shared_ptr<SoundDriver> KOSWindow::create_sound_driver() {
    return std::make_shared<KOSSoundDriver>(this);
}

}
