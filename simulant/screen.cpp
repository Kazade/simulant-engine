#include <chrono>

#include "window.h"
#include "screen.h"

#include "threads/future.h"

namespace smlt {


void Screen::render(const uint8_t *data, ScreenFormat format) {

    static thread::Future<void> fut;

    if(format != this->format()) {
        S_WARN("Not uploading screen image due to format mismatch. Conversion not yet supported");
        return;
    }

    buffer_.resize(data_size());
    buffer_.assign(data, data + data_size());
}

void Screen::update(float dt) {
    time_till_next_refresh_ -= dt;
    if(time_till_next_refresh_ <= 0.0f) {
        time_till_next_refresh_ = smlt::fast_divide(1.0f, refresh_rate_);

        /* Now if this blocks, it doesn't matter. The main thread
         * can continue to update buffer_ without waiting */
        window_->render_screen(this, &buffer_[0]);
    }
}

uint32_t Screen::data_size() const {
    if(format_ == SCREEN_FORMAT_G1) {
        return (width() * height()) / 8;
    } else {
        S_ERROR("Unknown format");
        return 1;
    }
}

Screen::Screen(Window *window, const std::string &name):
    window_(window),
    name_(name) {

}

std::string Screen::name() const {
    return name_;
}

}
