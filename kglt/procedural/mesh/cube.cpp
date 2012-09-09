#include "cube.h"

namespace kglt {
namespace procedural {
namespace mesh {

void cube(kglt::Mesh& mesh, float width) {
    mesh.vertices().clear();
    mesh.triangles().clear();

    float hw = width * 0.5;

    mesh.add_vertex(-hw, -hw, hw);
    mesh.add_vertex( hw, -hw, hw);
    mesh.add_vertex( hw,  hw, hw);
    mesh.add_vertex(-hw,  hw, hw);

    mesh.add_vertex(-hw, -hw,-hw);
    mesh.add_vertex( hw, -hw,-hw);
    mesh.add_vertex( hw,  hw,-hw);
    mesh.add_vertex(-hw,  hw,-hw);

    kglt::Triangle& tri1 = mesh.add_triangle(0, 1, 2);
    tri1.set_uv(0, 0.0, 0.0);
    tri1.set_uv(1, 1.0, 0.0);
    tri1.set_uv(2, 1.0, 1.0);

    tri1.set_normal(0, 0.0, 0.0, 1.0);
    tri1.set_normal(1, 0.0, 0.0, 1.0);
    tri1.set_normal(2, 0.0, 0.0, 1.0);

    kglt::Triangle& tri2 = mesh.add_triangle(0, 2, 3);
    tri2.set_uv(0, 0.0, 0.0);
    tri2.set_uv(1, 1.0, 1.0);
    tri2.set_uv(2, 0.0, 1.0);

    tri2.set_normal(0, 0.0, 0.0, 1.0);
    tri2.set_normal(1, 0.0, 0.0, 1.0);
    tri2.set_normal(2, 0.0, 0.0, 1.0);

    //Right side
    kglt::Triangle& tri3 = mesh.add_triangle(1, 5, 6);
    tri3.set_uv(0, 0.0, 0.0);
    tri3.set_uv(1, 1.0, 1.0);
    tri3.set_uv(2, 0.0, 1.0);

    kglt::Triangle& tri4 = mesh.add_triangle(1, 6, 2);
    tri4.set_uv(0, 0.0, 0.0);
    tri4.set_uv(1, 1.0, 1.0);
    tri4.set_uv(2, 0.0, 1.0);

    mesh.done();
}

}
}
}

