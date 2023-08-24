#pragma once

/* This is the window implementation for the Dreamcast using KallistiOS */

#include <kos.h>

#include "window.h"
#include "platform.h"

#include "threads/mutex.h"

namespace smlt {

struct KOSWindowPrivate;

class KOSWindow : public Window {
public:
    static Window::ptr create(Application* app) {
        return Window::create<KOSWindow>(app);
    }

    KOSWindow();

    void set_title(const std::string&) override {} // No-op
    void cursor_position(int32_t&, int32_t&) override {} // No-op
    void show_cursor(bool) override {} // No-op
    void lock_cursor(bool) override {} // No-op

    void swap_buffers() override;
    void destroy_window() override;
    void check_events() override;

    void initialize_input_controller(InputState &controller) override;

    std::shared_ptr<SoundDriver> create_sound_driver(const std::string& from_config) override;

private:
    bool _init_window() override;
    bool _init_renderer(Renderer* renderer) override;

    void probe_vmus();

    void render_screen(Screen* screen, const uint8_t* data, int row_stride) override;

    float time_since_last_controller_update_ = 0.0f;

    struct ControllerState {
        int8_t joyx = 0;
        int8_t joyy = 0;
        int8_t joyx2 = 0;
        int8_t joyy2 = 0;
        uint8_t ltrig = 0;
        uint8_t rtrig = 0;

        Seconds current_rumble_remaining_;
    };

    ControllerState previous_controller_state_[4] = {0};
    uint32_t previous_controller_button_state_[4] = {0};
    uint8_t previous_key_state_[MAX_KEYBOARD_CODES] = {0}; // value-initialize to zero

    virtual void game_controller_start_rumble(GameController *controller, RangeValue<0, 1> low_rumble, RangeValue<0, 1> high_rumble, const smlt::Seconds& duration) override;
    virtual void game_controller_stop_rumble(GameController *controller) override;

    friend struct KOSWindowPrivate;
    std::shared_ptr<KOSWindowPrivate> private_;
};

}
