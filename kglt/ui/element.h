#ifndef ELEMENT_H
#define ELEMENT_H

#include "../object.h"
#include "../generic/visitor.h"

namespace kglt {
namespace ui {

enum PaddingIndex {
    PADDING_LEFT = 0,
    PADDING_BOTTOM,
    PADDING_RIGHT,
    PADDING_TOP,
    PADDING_MAX
};

class Element :
    public Object,
    public generic::VisitableBase<Element> {

public:
    Element(Scene* scene);

    double total_width() { return padding_left() + padding_right() + width(); }
    double total_height() { return padding_top() + padding_bottom() + height(); }

    virtual double width() { return width_; }
    virtual double height() { return height_; }
    double border_width() const { return border_width_; }

    double padding_left() const { return padding_[PADDING_LEFT]; }
    double padding_right() const { return padding_[PADDING_RIGHT]; }
    double padding_top() const { return padding_[PADDING_TOP]; }
    double padding_bottom() const { return padding_[PADDING_BOTTOM]; }

    void set_background_colour(float r, float g, float b, float a);
    void set_foreground_colour(float r, float g, float b, float a);
    void set_border_colour(float r, float g, float b, float a);

    void set_position(float x, float y);

    Object& background();
    Object& border();
private:
    double width_;
    double height_;
    double border_width_;
    double padding_[PADDING_MAX];

    kglt::MeshID background_mesh_;
    kglt::MeshID border_mesh_;

protected:
    void rebuild_meshes();
    void _initialize(Scene& scene);
};

}
}


#endif // ELEMENT_H
