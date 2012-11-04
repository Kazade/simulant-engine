#include "cube.h"

namespace kglt {
namespace procedural {
namespace mesh {

void cube(kglt::Mesh& mesh, float width) {
    mesh.clear();

    float hw = width * 0.5;

    //Front
    mesh.shared_data().position(-hw, -hw, hw);
    mesh.shared_data().normal(0, 0, 1);
    mesh.shared_data().tex_coord0(0, 0);
    mesh.shared_data().move_next();

    mesh.shared_data().position( hw, -hw, hw);
    mesh.shared_data().normal(0, 0, 1);
    mesh.shared_data().tex_coord0(1, 0);
    mesh.shared_data().move_next();

    mesh.shared_data().position( hw,  hw, hw);
    mesh.shared_data().normal(0, 0, 1);
    mesh.shared_data().tex_coord0(1, 1);
    mesh.shared_data().move_next();

    mesh.shared_data().position(-hw,  hw, hw);
    mesh.shared_data().normal(0, 0, 1);
    mesh.shared_data().tex_coord0(0, 1);
    mesh.shared_data().move_next();

    //Right
    mesh.shared_data().position(hw, -hw, hw);
    mesh.shared_data().normal(1, 0, 0);
    mesh.shared_data().tex_coord0(0, 0);
    mesh.shared_data().move_next();

    mesh.shared_data().position( hw, -hw, -hw);
    mesh.shared_data().normal(1, 0, 0);
    mesh.shared_data().tex_coord0(1, 0);
    mesh.shared_data().move_next();

    mesh.shared_data().position( hw,  hw, -hw);
    mesh.shared_data().normal(0, 0, 1);
    mesh.shared_data().tex_coord0(1, 1);
    mesh.shared_data().move_next();

    mesh.shared_data().position(hw,  hw, hw);
    mesh.shared_data().normal(0, 0, 1);
    mesh.shared_data().tex_coord0(0, 1);
    mesh.shared_data().move_next();

    //Back
    mesh.shared_data().position(hw, -hw, -hw);
    mesh.shared_data().normal(1, 0, 0);
    mesh.shared_data().tex_coord0(0, 0);
    mesh.shared_data().move_next();

    mesh.shared_data().position(-hw, -hw, -hw);
    mesh.shared_data().normal(1, 0, 0);
    mesh.shared_data().tex_coord0(1, 0);
    mesh.shared_data().move_next();

    mesh.shared_data().position(-hw,  hw, -hw);
    mesh.shared_data().normal(0, 0, 1);
    mesh.shared_data().tex_coord0(1, 1);
    mesh.shared_data().move_next();

    mesh.shared_data().position(hw,  hw, -hw);
    mesh.shared_data().normal(0, 0, 1);
    mesh.shared_data().tex_coord0(0, 1);
    mesh.shared_data().move_next();

    mesh.shared_data().done();

    SubMeshIndex sm = mesh.new_submesh(MaterialID(), MESH_ARRANGEMENT_LINES, true);

    for(uint8_t i = 0; i < mesh.shared_data().count(); ++i) {
        mesh.submesh(sm).index_data().index(i);
    }
}

}
}
}

