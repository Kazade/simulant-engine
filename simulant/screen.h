#pragma once

#include <vector>

#include "generic/managed.h"
#include "generic/data_carrier.h"
#include "threads/mutex.h"

namespace smlt {

class Window;

enum ScreenFormat {
    SCREEN_FORMAT_G1, /* 1 grey bit per pixel */
    SCREEN_FORMAT_MAX
};

class Screen:
    public RefCounted<Screen>,
    public generic::DataCarrier {

public:
    Screen(Window* window, const std::string& name, uint16_t w, uint16_t h, ScreenFormat format=SCREEN_FORMAT_G1, uint16_t refresh=60);

    /* Render image data. data must be of size width x height x bits where bits
     * is defined by the ScreenFormat. Data should be arranged from top-left.
     *
     * Rendering is not instantaneous, submitting images too quickly could result
     * in some images being skipped.
     *
     * refresh_rate will give you some indication of the rate to submit images,
     * but if you experience frame skipping then reduce the update time.
     *
     * You can use a Scene fixed_update, a Behaviour or window->idle->add_timeout
     * to update with a regular frequency.
     */
    void render(const uint8_t* data);

    uint16_t height() const {
        return height_;
    }

    uint16_t width() const {
        return width_;
    }

    ScreenFormat format() const {
        return format_;
    }

    uint16_t refresh_rate() const {
        return refresh_rate_;
    }

    /*
     * If this is greater than one, then
     * all data sent via render is integer scaled
     */
    uint16_t integer_scale() const {
        return integer_scale_;
    }

    /* Private API, should only be called by the window
     * class that knows how to handle it */
    void _set_integer_scale(uint8_t scale) {
        integer_scale_ = scale;
    }

    std::string name() const;

    void update(float dt);

private:
    Window* window_;
    std::string name_;

    uint16_t width_ = 0;
    uint16_t height_ = 0;
    uint16_t row_stride_ = 0;
    ScreenFormat format_ = SCREEN_FORMAT_G1;
    uint16_t refresh_rate_ = 15;
    uint8_t integer_scale_ = 1;
    float time_till_next_refresh_ = 0.0f;
    bool buffer_dirty_ = false;

    std::vector<uint8_t> buffer_;
    thread::Mutex buffer_mutex_;

    friend class Window;
};

}
