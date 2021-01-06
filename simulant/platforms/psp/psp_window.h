#pragma once

#include <GLES/egl.h>

#include "../../window.h"
#include "../../platform.h"

namespace smlt {

class PSPWindow : public Window {
    class PSPPlatform : public Platform {
    public:
        std::string name() const override { return "psp"; }
    };

public:
    static Window::ptr create(Application* app, int width, int height, int bpp, bool fullscreen, bool enable_vsync) {
        return Window::create<PSPWindow>(app, width, height, bpp, fullscreen, enable_vsync);
    }

    PSPWindow(uint32_t width, uint32_t height, uint32_t bpp, bool fullscreen, bool vsync_enabled);

    void set_title(const std::string&) override {} // No-op
    void cursor_position(int32_t&, int32_t&) override {} // No-op
    void show_cursor(bool) override {} // No-op
    void lock_cursor(bool) override {} // No-op

    void swap_buffers() override;
    bool create_window() override;
    void destroy_window() override;
    void check_events() override;

    void initialize_input_controller(InputState &controller) override;

    std::shared_ptr<SoundDriver> create_sound_driver(const std::string& from_config) override;

private:
    EGLDisplay dpy_;
    EGLSurface surface_;
    EGLContext ctx_;

    void render_screen(Screen*, const uint8_t*) override {}
};

}
