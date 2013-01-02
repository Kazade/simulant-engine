#include "rectangle.h"

namespace kglt {
namespace procedural {
namespace mesh {

SubMeshIndex rectangle(kglt::Mesh& mesh, float width, float height, float x_offset, float y_offset, float z_offset, bool clear) {
    if(clear) {
        mesh.clear();
    }

    uint16_t offset = mesh.shared_data().count();

    mesh.shared_data().move_to_end();

    //Build some shared vertex data
    mesh.shared_data().position(x_offset + (-width / 2.0), y_offset + (-height / 2.0), z_offset);
    mesh.shared_data().diffuse(kglt::Colour::white);
    mesh.shared_data().tex_coord0(0.0, 0.0);
    mesh.shared_data().tex_coord1(0.0, 0.0);
    mesh.shared_data().tex_coord2(0.0, 0.0);
    mesh.shared_data().tex_coord3(0.0, 0.0);
    mesh.shared_data().normal(0, 0, 1);
    mesh.shared_data().move_next();

    mesh.shared_data().position(x_offset + (width / 2.0), y_offset + (-height / 2.0), z_offset);
    mesh.shared_data().diffuse(kglt::Colour::white);
    mesh.shared_data().tex_coord0(1.0, 0.0);
    mesh.shared_data().tex_coord1(1.0, 0.0);
    mesh.shared_data().tex_coord2(1.0, 0.0);
    mesh.shared_data().tex_coord3(1.0, 0.0);
    mesh.shared_data().normal(0, 0, 1);
    mesh.shared_data().move_next();

    mesh.shared_data().position(x_offset + (width / 2.0),  y_offset + (height / 2.0), z_offset);
    mesh.shared_data().diffuse(kglt::Colour::white);
    mesh.shared_data().tex_coord0(1.0, 1.0);
    mesh.shared_data().tex_coord1(1.0, 1.0);
    mesh.shared_data().tex_coord2(1.0, 1.0);
    mesh.shared_data().tex_coord3(1.0, 1.0);
    mesh.shared_data().normal(0, 0, 1);
    mesh.shared_data().move_next();

    mesh.shared_data().position(x_offset + (-width / 2.0),  y_offset + (height / 2.0), z_offset);
    mesh.shared_data().diffuse(kglt::Colour::white);
    mesh.shared_data().tex_coord0(0.0, 1.0);
    mesh.shared_data().tex_coord1(0.0, 1.0);
    mesh.shared_data().tex_coord2(0.0, 1.0);
    mesh.shared_data().tex_coord3(0.0, 1.0);
    mesh.shared_data().normal(0, 0, 1);
    mesh.shared_data().move_next();
    mesh.shared_data().done();

    //Create a submesh that uses the shared data
    SubMeshIndex sm = mesh.new_submesh(MaterialID(), MESH_ARRANGEMENT_TRIANGLES, true);
    mesh.submesh(sm).index_data().index(offset + 0);
    mesh.submesh(sm).index_data().index(offset + 1);
    mesh.submesh(sm).index_data().index(offset + 2);

    mesh.submesh(sm).index_data().index(offset + 0);
    mesh.submesh(sm).index_data().index(offset + 2);
    mesh.submesh(sm).index_data().index(offset + 3);    
    mesh.submesh(sm).index_data().done();

    return sm;
}

SubMeshIndex rectangle_outline(kglt::Mesh& mesh, float width, float height, float x_offset, float y_offset, float z_offset, bool clear) {
    if(clear) {
        mesh.clear();
    }

    uint16_t offset = mesh.shared_data().count();

    mesh.shared_data().position(x_offset + (-width / 2.0), y_offset + (-height / 2.0), z_offset);
    mesh.shared_data().diffuse(kglt::Colour::white);
    mesh.shared_data().tex_coord0(0.0, 0.0);
    mesh.shared_data().tex_coord1(0.0, 0.0);
    mesh.shared_data().move_next();

    mesh.shared_data().position(x_offset + (width / 2.0), y_offset + (-height / 2.0), z_offset);
    mesh.shared_data().diffuse(kglt::Colour::white);
    mesh.shared_data().tex_coord0(1.0, 0.0);
    mesh.shared_data().tex_coord1(1.0, 0.0);
    mesh.shared_data().move_next();

    mesh.shared_data().position(x_offset + (width / 2.0), y_offset + (height / 2.0), z_offset);
    mesh.shared_data().diffuse(kglt::Colour::white);
    mesh.shared_data().tex_coord0(1.0, 1.0);
    mesh.shared_data().tex_coord1(1.0, 1.0);
    mesh.shared_data().move_next();

    mesh.shared_data().position(x_offset + (-width / 2.0), y_offset + (height / 2.0), z_offset);
    mesh.shared_data().diffuse(kglt::Colour::white);
    mesh.shared_data().tex_coord0(0.0, 1.0);
    mesh.shared_data().tex_coord1(0.0, 1.0);
    mesh.shared_data().move_next();
    mesh.shared_data().done();
    
    SubMeshIndex sm = mesh.new_submesh(MaterialID(), MESH_ARRANGEMENT_LINE_STRIP, true);

    for(uint8_t i = 0; i < 4; ++i) {
        mesh.submesh(sm).index_data().index(offset + i);
    }

    //Add the original point
    mesh.submesh(sm).index_data().index(offset);
    mesh.submesh(sm).index_data().done();

    return sm;
}

}
}
}
