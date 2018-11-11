#include "window.h"
#include "screen.h"

namespace smlt {

void Screen::render(const uint8_t *data, ScreenFormat format) {
    if(format != this->format()) {
        L_WARN("Not uploading screen image due to format mismatch. Conversion not yet supported");
        return;
    }

    window_->render_screen(this, data);
}

Screen::Screen(Window *window):
    window_(window) {}

}
