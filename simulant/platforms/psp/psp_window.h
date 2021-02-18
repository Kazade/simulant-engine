#pragma once

#include <GLES/egl.h>

#include "../../window.h"
#include "../../platform.h"

namespace smlt {

class PSPWindow : public Window {
    class PSPPlatform : public Platform {
    public:
        std::string name() const override {
            return "psp";
        }

        Resolution native_resolution() const override {
            Resolution native;
            native.width = 480;
            native.height = 272;
            native.refresh_rate = 60; // FIXME?
            return native;
        }

        uint64_t available_ram_in_bytes() const override {
            return MEMORY_VALUE_UNAVAILABLE;
        }

        uint64_t total_ram_in_bytes() const override {
            return MEMORY_VALUE_UNAVAILABLE;
        }

        uint64_t available_vram_in_bytes() const override {
            return MEMORY_VALUE_UNAVAILABLE;
        }

        uint64_t process_ram_usage_in_bytes(ProcessID) const override {
            return MEMORY_VALUE_UNAVAILABLE;
        }
    };

public:
    const static PSPPlatform platform;

    static Window::ptr create(Application* app) {
        return Window::create<PSPWindow>(app);
    }

    PSPWindow();

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
    EGLDisplay dpy_;
    EGLSurface surface_;
    EGLContext ctx_;

    void render_screen(Screen*, const uint8_t*) override {}

    bool _init_window() override;
    bool _init_renderer(Renderer *renderer);
};

}
