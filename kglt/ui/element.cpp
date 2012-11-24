
#include "element.h"
#include "../mesh.h"
#include "../procedural/mesh.h"
#include "../scene.h"
#include "../window_base.h"
#include "../ui.h"

namespace kglt {
namespace ui {

Element::Element(UI *ui):
    Object(&ui->scene()),
    ui_(*ui),
    width_(0),
    height_(0),
    border_width_(1),
    padding_{5, 5, 5, 5} {

}

void Element::_initialize(Scene& scene) {
    mesh_id_ = scene.new_mesh();
    entity_id_ = scene.new_entity(mesh_id_);

    rebuild_meshes();
}

void Element::rebuild_meshes() {
    kglt::Mesh& mesh = scene().mesh(mesh_id_);

    //Create the background submesh
    SubMeshIndex background = kglt::procedural::mesh::rectangle(
        mesh,
        total_width(),
        total_height(),
        total_width() / 2.0,
        total_height() / 2.0,
        0.0, true
    );

    //Create an outline submesh (don't clear the existing submeshes or data)
    //Slightly offset in the z-axis
    SubMeshIndex border = kglt::procedural::mesh::rectangle_outline(
        mesh,
        total_width(),
        total_height(),
        total_width() / 2.0,
        total_height() / 2.0,
        0.1, false
    );

    //Go through the background submesh vertices and override the diffuse colour
    for(uint16_t idx: mesh.submesh(background).index_data().all()) {
        mesh.shared_data().move_to(idx);
        mesh.shared_data().diffuse(kglt::Colour(0.0, 0.2, 1.0, 0.75));
    }

    //Go through the border submesh vertices and override the diffuse colour
    for(uint16_t idx: mesh.submesh(border).index_data().all()) {
        mesh.shared_data().move_to(idx);
        mesh.shared_data().diffuse(kglt::Colour(0.0, 0.0, 0.5, 1.0));
    }

    //Rebuild the entity
    kglt::Entity& e = scene().entity(entity_id_);
    e.set_mesh(mesh_id_);
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

    Object::set_position(
        Vec3(
            parent_left + (x * parent_width),
            parent_bottom + (y * parent_height),
            position().z
        )
    );

    update_from_parent();
}

}
}
