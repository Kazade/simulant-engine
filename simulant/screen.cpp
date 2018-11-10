#include "window.h"
#include "screen.h"

namespace smlt {

void Screen::render(const uint8_t *data) {
    window_->render_screen(this, data);
}

Screen::Screen(Window *window):
    window_(window) {}

}
