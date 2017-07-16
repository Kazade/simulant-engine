#include <memory>
#include <vector>
#include <GL/gl.h>
#include <GL/glut.h>

#include "input_controller.h"
#include "kos_window.h"
#include "sound_drivers/kos_sound_driver.h"
#include "renderers/renderer_config.h"
#include "utils/memory.h"

namespace smlt {

KOS_INIT_FLAGS(INIT_DEFAULT | INIT_MALLOCSTATS);

#define SCREEN_WIDTH 640
#define SCREEN_HEIGHT 480
#define SCREEN_DEPTH 32

KOSWindow::KOSWindow(uint32_t width, uint32_t height, uint32_t bpp, bool fullscreen):
    WindowBase() {

    set_width(SCREEN_WIDTH);
    set_height(SCREEN_HEIGHT);
    set_bpp(SCREEN_DEPTH);
    set_fullscreen(true);
}


void KOSWindow::swap_buffers() {
    glutSwapBuffers();
}

bool KOSWindow::create_window(int width, int height, int bpp, bool fullscreen) {
    set_width(SCREEN_WIDTH);
    set_height(SCREEN_HEIGHT);

    L_DEBUG("Initializing OpenGL");
#ifdef _arch_dreamcast
        print_available_ram();
#endif

    glKosInit();
    L_DEBUG("OpenGL initialized");
#ifdef _arch_dreamcast
        print_available_ram();
#endif

    renderer_ = std::make_shared<GL1XRenderer>(this);

    set_has_context(true); //Mark that we have a valid GL context

    L_DEBUG("Renderer initialized");
#ifdef _arch_dreamcast
        print_available_ram();
#endif

    return true;
}

void KOSWindow::destroy_window() {

}

void KOSWindow::check_events() {
    // The weird ordering is to match the order they are defined in KOS
    const static std::vector<uint16_t> CONTROLLER_BUTTONS = {
        CONT_C, CONT_B, CONT_A, CONT_START, CONT_Z, CONT_Y, CONT_X, CONT_D
    };

    static std::array<uint16_t, 4> previous_controller_button_state = {{0, 0, 0, 0}};

    /* Check controller states */
    for(int8_t i = 0; i < 4; +i) {
        auto device = maple_enum_type(i, MAPLE_FUNC_CONTROLLER);
        if(device) {
            auto state = (cont_state_t*) maple_dev_status(device);
            if(state) {
                auto button_state = state->buttons;
                auto prev_state = previous_controller_button_state[i];

                // Check the current button state against the previous one
                // and update the input controller appropriately
                for(auto button: CONTROLLER_BUTTONS) {
                    if((button_state & button) && !(prev_state & button)) {
                        // Button down
                        input_controller()._handle_joypad_button_down(
                            i, button
                        );
                    }

                    if(!(button_state & button) && (prev_state & button)) {
                        // Button up
                        input_controller()._handle_joypad_button_up(
                            i, button
                        );
                    }
                }

                previous_controller_button_state[i] = button_state;
            }
        }
    }
}

std::shared_ptr<SoundDriver> KOSWindow::create_sound_driver() {
    return std::make_shared<KOSSoundDriver>(this);
}

void smlt::KOSWindow::initialize_input_controller(smlt::InputController &controller) {
    L_DEBUG("Detecting input devices");

    std::vector<GameControllerInfo> joypads;

    auto mouse_dev = maple_enum_type(0, MAPLE_FUNC_MOUSE);
    if(mouse_dev) {
        MouseControllerInfo mouse;
        mouse.id = 0;
        mouse.button_count = 3;
        controller._update_mouse_devices({mouse});
    }

    auto keyboard_dev = maple_enum_type(0, MAPLE_FUNC_KEYBOARD);
    if(keyboard_dev) {
        KeyboardControllerInfo keyboard;
        keyboard.id = 0;
        controller._update_keyboard_devices({keyboard});
    }

    auto controller_count = 0u;
    for(int8_t i = 0; i < 4; ++i) {
        auto device = maple_enum_type(i, MAPLE_FUNC_CONTROLLER);
        if(device) {
            GameControllerInfo info;
            info.id = i;
            info.name = device->info.product_name;
            joypads.push_back(info);

            controller_count++;
        }
    }

    controller._update_joypad_devices(joypads);

    L_DEBUG(_F("Found {0} connected controllers").format(controller_count));
}

}
