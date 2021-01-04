
#include "psp_window.h"
#include "../../sound_drivers/null_sound_driver.h"

namespace smlt {

#define SCREEN_WIDTH 480
#define SCREEN_HEIGHT 272
#define SCREEN_DEPTH 32


PSPWindow::PSPWindow(uint32_t width, uint32_t height, uint32_t bpp, bool fullscreen, bool vsync_enabled):
    Window(
        width ? std::min(width, (uint32_t) SCREEN_WIDTH) : SCREEN_WIDTH,
        height ? std::min(height, (uint32_t) SCREEN_HEIGHT) : SCREEN_HEIGHT,
        bpp ? bpp : SCREEN_DEPTH, true, true) {

    platform_.reset(new PSPPlatform);
}

void PSPWindow::swap_buffers() {

}

bool PSPWindow::create_window() {
    return true;
}

void PSPWindow::destroy_window() {

}

void PSPWindow::check_events() {

}

void PSPWindow::initialize_input_controller(InputState &controller) {

}

std::shared_ptr<SoundDriver> PSPWindow::create_sound_driver(const std::string& from_config) {
    _S_UNUSED(from_config);

    L_DEBUG("Null sound driver activated");
    return std::make_shared<NullSoundDriver>(this);
}

}
