#pragma once

/* This is the window implementation for the Dreamcast using KallistiOS */

#include <kos.h>

#include "window.h"
#include "platform.h"

namespace smlt {

class KOSWindow : public Window {
    class DreamcastPlatform : public Platform {
    public:
        std::string name() const override { return "dreamcast"; }
        void sleep_ms(uint32_t ms) const {
            usleep(ms * 1000);
        }
    };

    DreamcastPlatform platform_;

public:
    const Platform* platform() const override { return &platform_; }

    static Window::ptr create(Application* app, int width, int height, int bpp, bool fullscreen, bool enable_vsync) {
        return Window::create<KOSWindow>(app, width, height, bpp, fullscreen, enable_vsync);
    }

    KOSWindow(uint32_t width, uint32_t height, uint32_t bpp, bool fullscreen, bool vsync_enabled);

    void set_title(const std::string&) override {} // No-op
    void cursor_position(int32_t &mouse_x, int32_t &mouse_y) override {} // No-op
    void show_cursor(bool) override {} // No-op
    void lock_cursor(bool) override {} // No-op

    void swap_buffers() override;
    bool create_window() override;
    void destroy_window() override;
    void check_events() override;

    void initialize_input_controller(InputState &controller) override;

    std::shared_ptr<SoundDriver> create_sound_driver() override;
};

}
