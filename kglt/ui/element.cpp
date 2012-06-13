
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
    padding_({2, 2, 2, 2}),
    background_mesh_(0),
    border_mesh_(0) {

}

void Element::on_parent_set(Object* old_parent) {
    if(!border_mesh_) {
        border_mesh_ = scene().new_mesh();
    }

    if(!background_mesh_) {
        background_mesh_ = scene().new_mesh();
    }

    rebuild_meshes();
}

void Element::rebuild_meshes() {
    kglt::Mesh& border = scene().mesh(border_mesh_);
    border.set_parent(this);
    border.move_to(0.0, 0.0, 0.1); //Move the border slightly forward
    kglt::procedural::mesh::rectangle_outline(border, total_width(), total_height());

    for(kglt::Vertex& v: border.vertices()) {
        v.x -= total_width() / 2.0;
        v.y *= -1.0; //Flip the Y-axis
        v.y -= total_height() / 2.0;
    }

    border.set_diffuse_colour(kglt::Colour(1.0, 1.0, 1.0, 1.0));


    //CONTINUE
    /**
      1. Need to move the mesh so that it starts at 0,0 -> width, height
      2. Need to generate the background mesh
      3. Need to properly set the colours etc.
      4. Need to finish the label and make sure everything renders
    */
}

void Element::set_position(float x, float y) {
    double parent_width = 0.0;
    double parent_height = 0.0;
    double parent_left = 0.0;
    double parent_top = 0.0;

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
        parent_top = parent().position().y;
    } else {
        parent_width = scene().window().width();
        parent_height = scene().window().height();
    }

    position().x = parent_left + (x * parent_width);
    position().y = parent_top + (y * parent_height);
}

}
}
