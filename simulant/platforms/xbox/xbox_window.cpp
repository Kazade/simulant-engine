
#include "xbox_window.h"

#include "../../sound/drivers/null_sound_driver.h"

#include "../../renderers/renderer_config.h"

namespace smlt {

#define SCREEN_WIDTH 1280
#define SCREEN_HEIGHT 720
#define SCREEN_DEPTH 32


bool XBOXWindow::_init_window() {
    set_width(SCREEN_WIDTH);
    set_height(SCREEN_HEIGHT);

    return true;
}

bool XBOXWindow::_init_renderer(Renderer *renderer) {
    _S_UNUSED(renderer);

    set_has_context(true); //Mark that we have a valid GL context
    return true;
}

void XBOXWindow::destroy_window() {

}

void XBOXWindow::check_events() {

        // FIXME: Other buttons
}

void XBOXWindow::initialize_input_controller(InputState &controller) {
    // GameControllerInfo info;
    // info.id = GameControllerID(0);
    // std::strncpy(info.name, "Internal", sizeof(info.name));
    // info.axis_count = 2;
    // info.button_count = 22;
    // info.hat_count = 0;
    // info.has_rumble = false;

    controller._update_keyboard_devices({});
    controller._update_mouse_devices({});
    controller._update_game_controllers({});
}

std::shared_ptr<SoundDriver> XBOXWindow::create_sound_driver(const std::string& from_config) {
    const char* from_env = std::getenv("SIMULANT_SOUND_DRIVER");

    std::string selected = (from_env) ? from_env :
        (from_config.empty()) ? "null" : from_config;

    S_DEBUG("Null sound driver activated");
    return std::make_shared<NullSoundDriver>(this);
}

}
