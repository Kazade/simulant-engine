#pragma once

/* This is the window implementation for the Dreamcast using KallistiOS */

#include <kos.h>

#include "window.h"
#include "platform.h"

#include "threads/mutex.h"

namespace smlt {

class KOSWindow : public Window {
public:
    static Window::ptr create(Application* app) {
        return Window::create<KOSWindow>(app);
    }

    KOSWindow() = default;

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

    /* Name, to port/unit combo. This only includes VMUs we've seen during the last probe */

    thread::Mutex vmu_mutex_;
    std::unordered_map<std::string, std::pair<int, int>> vmu_lookup_;

    float time_since_last_controller_update_ = 0.0f;
    float time_since_last_rumble_ = 0.25f;

    virtual void game_controller_start_rumble(GameController *controller, RangeValue<0, 1> low_rumble, RangeValue<0, 1> high_rumble, const smlt::Seconds& duration) override;
    virtual void game_controller_stop_rumble(GameController *controller) override;
};

}
