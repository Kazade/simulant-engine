#pragma once

/* This is the window implementation for the Dreamcast using KallistiOS */

#include <kos.h>

#include "window.h"
#include "platform.h"

#include "threads/mutex.h"

namespace smlt {

class KOSWindow : public Window {
    class DreamcastPlatform : public Platform {
    public:
        std::string name() const override {
            return "dreamcast";
        }

        Resolution native_resolution() const override {
            Resolution native;
            native.width = 640;
            native.height = 480;
            native.refresh_rate = 60;
            return native;
        }

        uint64_t available_ram_in_bytes() const override;
        uint64_t total_ram_in_bytes() const override;

        uint64_t available_vram_in_bytes() const override {
            return -1;
        }

        uint64_t process_ram_usage_in_bytes(ProcessID process_id) const override;
    };

public:
    static const DreamcastPlatform platform;

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

    void render_screen(Screen* screen, const uint8_t* data) override;

    /* Name, to port/unit combo. This only includes VMUs we've seen during the last probe */

    thread::Mutex vmu_mutex_;
    std::unordered_map<std::string, std::pair<int, int>> vmu_lookup_;
};

}
