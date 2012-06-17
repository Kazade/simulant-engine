
#include "element.h"
#include "../mesh.h"
#include "../procedural/mesh.h"
#include "../scene.h"
#include "../window_base.h"

namespace kglt {
namespace ui {

Element::Element():
    width_(0),
    height_(0),
    border_width_(1),
    padding_({5, 5, 5, 5}),
    background_mesh_(0),
    border_mesh_(0) {

}

void Element::_initialize(Scene& scene) {
    if(!border_mesh_) {
        border_mesh_ = scene.new_mesh();
    }

    if(!background_mesh_) {
        background_mesh_ = scene.new_mesh();
    }

    rebuild_meshes();
}

void Element::rebuild_meshes() {
    kglt::Mesh& border = scene().mesh(border_mesh_);
    border.set_parent(this);
    border.move_to(0.0, 0.0, 0.1); //Move the border slightly forward
    kglt::procedural::mesh::rectangle_outline(border, total_width(), total_height());

    for(kglt::Vertex& v: border.vertices()) {
        v.x += total_width() / 2.0 ;
        v.y += total_height() / 2.0;
    }

    border.set_diffuse_colour(kglt::Colour(0.0, 0.0, 0.5, 1.0));

    kglt::Mesh& background = scene().mesh(background_mesh_);
    background.set_parent(this);
    kglt::procedural::mesh::rectangle(background, total_width(), total_height());

    for(kglt::Vertex& v: background.vertices()) {
        v.x += total_width() / 2.0;
        v.y += total_height() / 2.0;
    }

    background.set_diffuse_colour(kglt::Colour(0.0, 0.2, 1.0, 0.75));
}

Object& Element::background() {
    return scene().mesh(background_mesh_);
}

Object& Element::border() {
    return scene().mesh(border_mesh_);
}

void Element::set_position(float x, float y) {
    double parent_width = 0.0;
    double parent_height = 0.0;
    double parent_left = 0.0;
    double parent_bottom = 0.0;

    ui::Element* parent_element = nullptr;
    if(has_parent()) {
        parent_element = dynamic_cast<ui::Element*>(&parent());
    }

    /*
      If the parent element is a ui::Element, then the position is relative to its
      top-left. Otherwise, the position is relative to 0,0 (top-left of the window)
    */
    if(parent_element) {
        parent_width = parent_element->width();
        parent_height = parent_element->height();
        parent_left = parent().position().x;
        parent_bottom = parent().position().y;
    } else {
        parent_width = scene().window().width();
        parent_height = scene().window().height();
    }

    position().x = parent_left + (x * parent_width);
    position().y = parent_bottom + (y * parent_height);
}

}
}
