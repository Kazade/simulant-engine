#include "kglt/kglt.h"
#include "kglt/shortcuts.h"

int main(int argc, char* argv[]) {
    kglt::Window::ptr window = kglt::Window::create();

    kglt::Scene& scene = window->scene();
	kglt::Mesh& mesh = kglt::return_new_mesh(scene);
	
	kglt::procedural::mesh::rectangle_outline(mesh, 1.0, 1.0);
	mesh.move_to(0.0, 0.0, -5.0);
	
    while(window->update()) {}
	
	return 0;
}
