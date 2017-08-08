/* *   Copyright (c) 2011-2017 Luke Benstead https://simulant-engine.appspot.com
 *
 *     This file is part of Simulant.
 *
 *     Simulant is free software: you can redistribute it and/or modify
 *     it under the terms of the GNU General Public License as published by
 *     the Free Software Foundation, either version 3 of the License, or
 *     (at your option) any later version.
 *
 *     Simulant is distributed in the hope that it will be useful,
 *     but WITHOUT ANY WARRANTY; without even the implied warranty of
 *     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *     GNU General Public License for more details.
 *
 *     You should have received a copy of the GNU General Public License
 *     along with Simulant.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef MOCK_WINDOW_H
#define MOCK_WINDOW_H

#include "../generic/managed.h"
#include "../window.h"

namespace smlt {
namespace testing {

class MockWindow :
    public Window {

public:
    MockWindow() {}

    sigc::signal<void, KeyCode>& signal_key_up() { return key_up_; }
    sigc::signal<void, KeyCode>& signal_key_down() { return key_down_; }

    void set_title(const std::string& title) {}
    void cursor_position(int32_t& mouse_x, int32_t& mouse_y) {}

    void check_events() {}
    void swap_buffers() {}

    static Window::ptr create(int width=640, int height=480, int bpp=0, bool fullscreen=false) {
        return Window::create<MockWindow>(width, height, bpp, fullscreen);
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
