#ifndef MOCK_WINDOW_H
#define MOCK_WINDOW_H

#include "../generic/managed.h"
#include "../window_base.h"

namespace smlt {
namespace testing {

class MockWindow :
    public WindowBase {

public:
    MockWindow() {}

    sigc::signal<void, KeyCode>& signal_key_up() { return key_up_; }
    sigc::signal<void, KeyCode>& signal_key_down() { return key_down_; }

    void set_title(const std::string& title) {}
    void cursor_position(int32_t& mouse_x, int32_t& mouse_y) {}

    void check_events() {}
    void swap_buffers() {}

    static WindowBase::ptr create(int width=640, int height=480, int bpp=0, bool fullscreen=false) {
        return WindowBase::create<MockWindow>(width, height, bpp, fullscreen);
    }

private:
    bool create_window(int width, int height, int bpp, bool fullscreen) {
        set_width(width);
        set_height(height);
        return true;
    }

    sigc::signal<void, KeyCode> key_up_;
    sigc::signal<void, KeyCode> key_down_;
};

}
}

#endif // MOCK_WINDOW_H
