#include "circle.h"

namespace kglt {
namespace procedural {
namespace mesh {

void circle(kglt::Mesh& mesh, float diameter, float x_offset, float y_offset, float z_offset, bool clear) {
    float radius = diameter * 0.5f;

    int point_count = 40;

    if(clear) {
        mesh.clear();
    }

    uint16_t offset = mesh.shared_data().count();

    mesh.shared_data().move_to_end();

    for(uint16_t i = 0; i < point_count; ++i) {
        //Build some shared vertex data

        float rads = 2 * kmPI * i / point_count;

        float x = radius * cos(rads);
        float y = radius * sin(rads);

        float u = cos(rads) * 0.5 + 0.5f;
        float v = sin(rads) * 0.5 + 0.5f;

        mesh.shared_data().position(x_offset + x, y_offset + y, z_offset);
        mesh.shared_data().diffuse(kglt::Colour::white);
        mesh.shared_data().tex_coord0(u, v);
        mesh.shared_data().tex_coord1(u, v);
        mesh.shared_data().tex_coord2(u, v);
        mesh.shared_data().tex_coord3(u, v);
        mesh.shared_data().normal(0, 0, 1);
        mesh.shared_data().move_next();
    }

    mesh.shared_data().done();

    SubMeshIndex sm = mesh.new_submesh(MaterialID(), MESH_ARRANGEMENT_TRIANGLE_FAN, true);

    for(uint16_t i = 1; i < point_count; ++i) {
        mesh.submesh(sm).index_data().index(offset + i);
    }
    mesh.submesh(sm).index_data().index(offset);
    mesh.submesh(sm).index_data().done();
}

void circle_outline(kglt::Mesh& mesh, float diameter, float x_offset, float y_offset, float z_offset, bool clear) {

}

}
}
}
