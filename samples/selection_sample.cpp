#include "kglt/kglt.h"
#include "kglt/shortcuts.h"

int main(int argc, char* argv[]) {
	kglt::Window window;	
	kglt::Scene& scene = window.scene();
	kglt::SelectionRenderer selection_renderer(new kglt::SelectionRenderer());	
	
	window.show_cursor(true); //Show the cursor
	
	scene.remove_all_passes();		
	scene.add_pass(selection_renderer); //Add the selection pass
	scene.add_pass(kglt::Renderer::ptr(new kglt::GenericRenderer())); //Normal rendering
		
	while(window.update()) {
		kglt::MeshID hovered_mesh = selection_renderer.selected_mesh();
		if(hovered_mesh) {
			scene.mesh(hovered_mesh).set_diffuse_colour(kglt::Colour(1.0, 0.0, 0.0, 1.0));
		}
	}
	
	return 0;
}
