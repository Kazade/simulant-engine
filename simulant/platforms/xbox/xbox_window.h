#pragma once

#include "../../window.h"
#include "../../platform.h"

namespace smlt {

class XBOXWindow : public Window {
public:
    static Window::ptr create(Application* app) {
        return Window::create<XBOXWindow>(app);
    }

    XBOXWindow() = default;

    void set_title(const std::string&) override {} // No-op
    void cursor_position(int32_t&, int32_t&) override {} // No-op
    void show_cursor(bool) override {} // No-op
    void lock_cursor(bool) override {} // No-op

    void destroy_window() override;
    void check_events() override;

    void initialize_input_controller(InputState &controller) override;

    std::shared_ptr<SoundDriver> create_sound_driver(const std::string& from_config) override;

private:
    void render_screen(Screen*, const uint8_t*, int) override {}

    bool _init_window() override;
    bool _init_renderer(Renderer *renderer) override;
};

}
