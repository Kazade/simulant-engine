
#include "psp_window.h"
#include "../../sound/drivers/null_sound_driver.h"
#include "../../renderers/renderer_config.h"

namespace smlt {

#define SCREEN_WIDTH 480
#define SCREEN_HEIGHT 272
#define SCREEN_DEPTH 32

void PS2Window::swap_buffers() {
    
}

bool PS2Window::_init_window() {
    // set_width(width);
    // set_height(height);

    return true;
}

bool PS2Window::_init_renderer(Renderer *renderer) {
    _S_UNUSED(renderer);

    set_has_context(true); //Mark that we have a valid GL context
    return true;
}

void PS2Window::destroy_window() {
    
}

void PS2Window::check_events() {

}

void PS2Window::initialize_input_controller(InputState &controller) {

}

std::shared_ptr<SoundDriver> PS2Window::create_sound_driver(const std::string& from_config) {
    _S_UNUSED(from_config);

    S_DEBUG("Null sound driver activated");
    return std::make_shared<NullSoundDriver>(this);
}

}
