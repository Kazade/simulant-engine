#pragma once

/* This is the window implementation for the Dreamcast using KallistiOS */

#include <kos.h>

#include "window_base.h"

namespace smlt {

class KOSWindow : public WindowBase {
public:
    static WindowBase::ptr create(Application* app, int width=640, int height=480, int bpp=0, bool fullscreen=false) {
        return WindowBase::create<KOSWindow>(app, width, height, bpp, fullscreen);
    }

    KOSWindow(uint32_t width, uint32_t height, uint32_t bpp, bool fullscreen);

    void set_title(const std::string&) override {} // No-op
    void cursor_position(int32_t &mouse_x, int32_t &mouse_y) override {} // No-op
    void show_cursor(bool) override {} // No-op

    void swap_buffers() override;
    bool create_window(int width, int height, int bpp, bool fullscreen) override;
    void destroy_window() override;
    void check_events() override;

    void initialize_input_controller(InputController &controller) override;

    std::shared_ptr<SoundDriver> create_sound_driver() override;
};

}
