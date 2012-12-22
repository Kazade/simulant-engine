#ifndef WIDGET_H
#define WIDGET_H

#include "../../generic/managed.h"
#include "../../types.h"

namespace kglt {
namespace extra {
namespace ui {

class Interface;

struct Ratio {
    explicit Ratio(float value):
        value_(value) {

        if(value < 0 && value > 1) {
            throw std::logic_error("Invalid argument passed to Ratio");
        }
    }

    float value_;
};

class Widget {
public:
    Widget(Interface& interface, Widget* parent, Ratio left, Ratio top, Ratio width, Ratio height);
    virtual ~Widget() {}

    void set_position(Ratio left, Ratio top);
    void set_background(TextureID tex);
    void set_padding(uint8_t left, uint8_t right, uint8_t bottom, uint8_t top);
    void set_border_width(uint8_t width);
    void set_border_colour(const Colour& colour);
    void set_background_colour(const Colour& colour);
    void set_foreground_colour(const Colour& colour);

    Interface& interface() { return interface_; }

private:
    Interface& interface_;
};

}
}
}

#endif // WIDGET_H
