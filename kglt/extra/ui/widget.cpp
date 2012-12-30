#include "interface.h"
#include "widget.h"
#include "../../scene.h"
#include "../../entity.h"
#include "../../procedural/mesh.h"

namespace kglt {
namespace extra {
namespace ui {

Widget::Widget(Interface& interface):
    interface_(interface),
    left_(Ratio(0.0)),
    top_(Ratio(0.0)),
    width_(Ratio(1.0)),
    height_(Ratio(1.0)),
    parent_(nullptr) {

    mesh_ = interface.subscene().new_mesh();
    entity_ = interface.subscene().new_entity();

    Mesh& mesh = interface.subscene().mesh(mesh_);

    background_ = kglt::procedural::mesh::rectangle(
        mesh,
        width_in_pixels(),
        height_in_pixels(),
        width_in_pixels() / 2,
        height_in_pixels() / 2,
        0
    );

    interface.subscene().entity(entity_).set_mesh(mesh_);
}

uint16_t Widget::width_in_pixels() const {
    return (parent_) ? parent_->width_in_pixels() * width_.value : interface_.width_in_pixels() * width_.value;
}

uint16_t Widget::height_in_pixels() const {
    return (parent_) ? parent_->height_in_pixels() * height_.value : interface_.height_in_pixels() * height_.value;
}

void Widget::set_position(Ratio left, Ratio top) {
    left_ = left;
    top_ = top;

    interface_.subscene().entity(entity_).set_position(Vec3(
        (parent_) ? parent_->width_in_pixels() * left_.value : interface_.width_in_pixels() * left_.value,
        (parent_) ? parent_->height_in_pixels() * top_.value : interface_.height_in_pixels() * top_.value,
    0)); //FIXME: z-index
}

void Widget::set_size(Ratio width, Ratio height) {
    width_ = width;
    height_ = height;

    Mesh& mesh = interface().subscene().mesh(mesh_);

    background_ = kglt::procedural::mesh::rectangle(
        mesh,
        width_in_pixels(),
        height_in_pixels(),
        width_in_pixels() / 2,
        height_in_pixels() / 2,
        0
    );

    interface().subscene().entity(entity_).set_mesh(mesh_);

    on_resize();
}

void Widget::set_foreground_colour(const Colour& colour) {
    foreground_ = colour;
    on_foreground_colour_changed();
}

void Widget::set_padding(uint8_t left, uint8_t right, uint8_t bottom, uint8_t top) {
    padding_.left = left;
    padding_.right = right;
    padding_.bottom = bottom;
    padding_.top = top;
}

}
}
}
