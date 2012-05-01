#include "rectangle.h"

namespace kglt {
namespace procedural {
namespace mesh {

void rectangle(kglt::Mesh& mesh, float width, float height) {
	mesh.vertices().clear();
	mesh.triangles().clear();
	
    mesh.add_vertex(-width / 2.0, -height / 2.0, 0.0);
    mesh.add_vertex(width / 2.0, -height / 2.0, 0.0);
    mesh.add_vertex(width / 2.0,  height / 2.0, 0.0);
    mesh.add_vertex(-width / 2.0,  height / 2.0, 0.0);

    kglt::Triangle& tri1 = mesh.add_triangle(0, 1, 2);
    tri1.set_uv(0, 0.0, 0.0);
    tri1.set_uv(1, 1.0, 0.0);
    tri1.set_uv(2, 1.0, 1.0);

    kglt::Triangle& tri2 = mesh.add_triangle(0, 2, 3);
    tri2.set_uv(0, 0.0, 0.0);
    tri2.set_uv(1, 1.0, 1.0);
    tri2.set_uv(2, 0.0, 1.0);
    
    mesh.done();
}

}
}
}
