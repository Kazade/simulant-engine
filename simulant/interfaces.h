#ifndef INTERFACES_H
#define INTERFACES_H

#include <vector>
#include "types.h"

namespace smlt {

class RenderableStage {
    /*
     *  Keeps track of the number of pipelines a stage of some kind is
     *  active on
     */
public:
    void increment_render_count() {
        render_count_++;
        if(render_count_ == 1) {
            on_render_started();
        }
    }

    void decrement_render_count() {
        render_count_--;
        assert(render_count_ >= 0);

        if(render_count_ == 0) {
            on_render_stopped();
        }
    }

    bool is_being_rendered() const { return bool(render_count_); }

private:
    int render_count_ = 0;

    virtual void on_render_started() = 0;
    virtual void on_render_stopped() = 0;
};


class RenderTarget {
public:
    virtual ~RenderTarget() {}

    virtual uint32_t width() const = 0;
    virtual uint32_t height() const = 0;

    virtual void set_clear_every_frame(uint32_t clear_flags=BUFFER_CLEAR_ALL, const smlt::Colour& colour=smlt::Colour::BLACK) {
        clear_flags_ = clear_flags;
        clear_colour_ = colour;
    }
    virtual uint32_t clear_every_frame_flags() const { return clear_flags_; }
    virtual smlt::Colour clear_every_frame_colour() const { return clear_colour_; }

private:
    uint32_t clear_flags_ = BUFFER_CLEAR_ALL;
    smlt::Colour clear_colour_ = smlt::Colour::BLACK;
};

}

#endif // INTERFACES_H
