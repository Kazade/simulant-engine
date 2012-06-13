#ifndef ELEMENT_H
#define ELEMENT_H

#include "../object.h"
#include "../object_visitor.h"

namespace kglt {
namespace ui {

enum PaddingIndex {
    PADDING_LEFT = 0,
    PADDING_BOTTOM,
    PADDING_RIGHT,
    PADDING_TOP,
    PADDING_MAX
};

class Element : public Object {
public:
    Element();

    void accept(ObjectVisitor& visitor) {
        visitor.pre_visit(this);

        for(Object* child: children_) {
            child->accept(visitor);
        }

        if(is_visible()) {
            visitor.visit(this);
        }
        visitor.post_visit(this);
    }

    double total_width() const { return padding_left() + padding_right() + width(); }
    double total_height() const { return padding_top() + padding_bottom() + height(); }

    virtual double width() const { return width_; }
    virtual double height() const { return height_; }
    double border_width() const { return border_width_; }

    double padding_left() const { return padding_[PADDING_LEFT]; }
    double padding_right() const { return padding_[PADDING_RIGHT]; }
    double padding_top() const { return padding_[PADDING_TOP]; }
    double padding_bottom() const { return padding_[PADDING_BOTTOM]; }

    void set_background_colour(float r, float g, float b, float a);
    void set_foreground_colour(float r, float g, float b, float a);
    void set_border_colour(float r, float g, float b, float a);

    void set_position(float x, float y);

private:
    double width_;
    double height_;
    double border_width_;
    double padding_[PADDING_MAX];

    kglt::MeshID background_mesh_;
    kglt::MeshID border_mesh_;

    void rebuild_meshes();

protected:
    void on_parent_set(Object *old_parent);
};

}
}


#endif // ELEMENT_H
