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

    SubMeshIndex sm = mesh.new_submesh(MaterialID(), MESH_ARRANGEMENT_TRIANGLES, true);

    //Go through each of the sides
    for(uint8_t side = 0; side < mesh.shared_data().count() / 4; ++side) {
        mesh.submesh(sm).index_data().index((side * 4));
        mesh.submesh(sm).index_data().index((side * 4) + 1);
        mesh.submesh(sm).index_data().index((side * 4) + 2);

        mesh.submesh(sm).index_data().index((side * 4));
        mesh.submesh(sm).index_data().index((side * 4) + 2);
        mesh.submesh(sm).index_data().index((side * 4) + 3);
    }

    mesh.submesh(sm).index_data().done();
}

}
}
}

