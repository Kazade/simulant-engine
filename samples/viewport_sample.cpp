#include "kglt/kglt.h"
#include "kglt/shortcuts.h"

int main(int argc, char* argv[]) {
	kglt::Window window;
	
	kglt::Scene& scene = window.scene();
	
	//scene.pass().viewport().configure(VIEWPORT_16_BY_9);
	
	scene.remove_all_passes();
	scene.add_pass(
        kglt::Renderer::ptr(new kglt::GenericRenderer()),
		kglt::VIEWPORT_TYPE_VERTICAL_SPLIT_LEFT
	);
	
	scene.add_pass(
        kglt::Renderer::ptr(new kglt::GenericRenderer()),
		kglt::VIEWPORT_TYPE_VERTICAL_SPLIT_RIGHT
	);
	
	scene.pass(0).viewport().set_background_colour(kglt::Colour(1.0, 0, 0, 0));
	scene.pass(1).viewport().set_background_colour(kglt::Colour(0, 1.0, 0, 0));
	
	//Create a cube mesh
	kglt::Mesh& mesh = kglt::return_new_mesh(scene);	
//	kglt::procedural::mesh::cube(mesh, 1.0);
	mesh.move_to(0.0, 0.0, -5.0);
		
	while(window.update()) {}
	
	return 0;
}
