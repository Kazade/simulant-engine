
#include "element.h"

#include "procedural/mesh.h"

namespace kglt {
namespace ui {

Element::Element():
    width_(0),
    height_(0),
    border_width_(1),
    padding_({2, 2, 2, 2}) {

    border_mesh_ = scene().new_mesh();
    background_mesh_ = scene().new_mesh();

    rebuild_meshes();
}

void Element::rebuild_meshes() {
    kglt::Mesh& border = scene().mesh(border_mesh_);
    border.set_parent(this);
    border.move_to(0.0, 0.0, 0.1); //Move the border slightly forward
    kglt::procedural::mesh::rectangle_outline(border, width(), height());

    //CONTINUE
    /**
      1. Need to move the mesh so that it starts at 0,0 -> width, height
      2. Need to generate the background mesh
      3. Need to properly set the colours etc.
      4. Need to finish the label and make sure everything renders
    */
}


}
}
