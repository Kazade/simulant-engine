#include <chrono>

#include "generic/simple_future.h"

#include "window.h"
#include "screen.h"

namespace smlt {

void Screen::render(const uint8_t *data, ScreenFormat format) {
    static std::future<void> future;

    if(format != this->format()) {
        L_WARN("Not uploading screen image due to format mismatch. Conversion not yet supported");
        return;
    }


    {
        std::lock_guard<std::mutex> lock(buffer_mutex_);

        buffer_.resize(data_size());
        buffer_.assign(data, data + data_size());
    }

    /* Is there already a valid future? */
    if(future.valid()) {
        /* If so, is it ready? */
        if(future.wait_for(std::chrono::milliseconds(0)) == std::future_status::ready) {
            /* Then get it, and mark it as not valid */
            future.get();
        }
    }

    /* We don't have a valid future, defer a new one */
    if(!future.valid()) {
        /* We async this and return immediately */
        future = std::async(std::launch::async, [this]() {
            std::vector<uint8_t> tmp;
            {
                /* Copy the buffer while locking */
                std::lock_guard<std::mutex> lock(buffer_mutex_);
                tmp = buffer_;
            }

            /* Now if this blocks, it doesn't matter. The main thread
             * can continue to update buffer_ without waiting */
            window_->render_screen(this, &tmp[0]);
        });
    }
}

uint32_t Screen::data_size() const {
    if(format_ == SCREEN_FORMAT_G1) {
        return (width() * height()) / 8;
    } else {
        L_ERROR("Unknown format");
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
