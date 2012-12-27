#ifndef WIDGET_H
#define WIDGET_H

#include "../../generic/managed.h"
#include "../../types.h"
#include "../../mesh.h"

namespace kglt {
namespace extra {
namespace ui {

class Interface;

struct Ratio {
    explicit Ratio(float value):
        value(value) {

        if(value < 0 && value > 1) {
            throw std::logic_error("Invalid argument passed to Ratio");
        }
    }

    float value;
};

struct Padding {
    Padding():
        left(5),
        right(5),
        bottom(5),
        top(5) {

    }

    uint8_t left;
    uint8_t right;
    uint8_t bottom;
    uint8_t top;
};

class Widget {
public:
    Widget(Interface& interface);
    virtual ~Widget() {}

    void set_position(Ratio left, Ratio top);
    void set_size(Ratio width, Ratio height);

    void set_background(TextureID tex);
    void set_padding(uint8_t left, uint8_t right, uint8_t bottom, uint8_t top);
    void set_border_width(uint8_t width);
    void set_border_colour(const Colour& colour);
    void set_background_colour(const Colour& colour);
    void set_foreground_colour(const Colour& colour);

    Interface& interface() { return interface_; }
    const Padding& padding() const { return padding_; }
    const Colour& foreground_colour() const { return foreground_; }

    uint16_t width_in_pixels() const;
    uint16_t height_in_pixels() const;

protected:
    EntityID entity_id() const { return entity_; }

    virtual void on_resize() {}
    virtual void on_foreground_colour_changed() {}

private:
    Interface& interface_;

    Ratio left_;
    Ratio top_;
    Ratio width_;
    Ratio height_;

    Padding padding_;
    Colour foreground_;

    MeshID mesh_;
    EntityID entity_;

    SubMeshIndex background_;
    SubMeshIndex border_;

    Widget* parent_;
};

}
}
}

#endif // WIDGET_H
