#ifndef MOCK_WINDOW_H
#define MOCK_WINDOW_H

#include "../generic/managed.h"
#include "../window_base.h"

namespace kglt {
namespace testing {

class MockWindow :
    public WindowBase,
    public Managed<MockWindow> {

public:
    sigc::signal<void, KeyCode>& signal_key_up() { return key_up_; }
    sigc::signal<void, KeyCode>& signal_key_down() { return key_down_; }

    void set_title(const std::string& title) {}
    void cursor_position(int32_t& mouse_x, int32_t& mouse_y) {}

    void check_events() {}
    void swap_buffers() {}

private:
    sigc::signal<void, KeyCode> key_up_;
    sigc::signal<void, KeyCode> key_down_;
};

}
}

#endif // MOCK_WINDOW_H
