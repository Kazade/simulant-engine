#pragma once

#include "generic/managed.h"


namespace smlt {

class Window;

enum ScreenFormat {
    SCREEN_FORMAT_G1, /* 1 grey bit per pixel */
    SCREEN_FORMAT_MAX
};

class Screen:
    public Managed<Screen> {

public:
    Screen(Window* window);

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

private:
    Window* window_;
    uint16_t width_ = 0;
    uint16_t height_ = 0;
    ScreenFormat format_ = SCREEN_FORMAT_G1;
    uint16_t refresh_rate_ = 60;

    friend class Window;
};

}
