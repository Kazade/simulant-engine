#include <hal/debug.h>
#include <hal/video.h>
#include <hal/xbox.h>

#include "xbox_window.h"

#include "../../renderers/gl_includes.h"
#include "../../sound/drivers/null_sound_driver.h"

#include "../../renderers/renderer_config.h"

namespace smlt {

bool XBOXWindow::_init_window() {
    return true;
}

bool XBOXWindow::_init_renderer(Renderer *renderer) {
    _S_UNUSED(renderer);

    S_DEBUG("Initialzing video mode and pbgl");

    auto w = renderer->window->width();
    auto h = renderer->window->height();
    XVideoSetMode(w, h, 32, REFRESH_DEFAULT);

    S_DEBUG("Video mode set to {0}x{1}", w, h);

    const int err = pbgl_init(GL_TRUE);
    if (err < 0) {
        S_DEBUG("pbgl_init() failed: {0}", err);
        XReboot();
        return false;
    }

    S_DEBUG("pbgl initialized successfully");
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

void XBOXWindow::do_swap_buffers() {
    pbgl_swap_buffers();
}

}
