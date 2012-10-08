#include "kglt/kglt.h"
#include "kglt/shortcuts.h"

int main(int argc, char* argv[]) {
	//Set up logging to stdio

/** FIXME:
 *  THIS SAMPLE IS TOTALLY BROKEN AS IT SHOULD BE IMPLEMENTED
 *  IN TERMS OF PASSES RATHER THAN A SEPARATE RENDERER
 *
	logging::get_logger("/")->add_handler(logging::Handler::ptr(new logging::StdIOHandler));
		
	kglt::Window window;	
	kglt::Scene& scene = window.scene();
	
	window.show_cursor(true); //Show the cursor
	
	scene.remove_all_passes();		
	
    auto selection = kglt::SelectionRenderer::create();
    scene.add_pass(selection); //Add the selection pass
    scene.add_pass(kglt::GenericRenderer::create()); //Normal rendering
	
    scene.active_camera().set_perspective_projection(
        45.0,
        float(window.width()) / float(window.height()),
        0.1,
        1000.0
    );

	kglt::Mesh& mesh_1 = kglt::return_new_mesh(scene);
	kglt::Mesh& mesh_2 = kglt::return_new_mesh(scene);
	
	kglt::procedural::mesh::rectangle(mesh_1, 1.0, 1.0);
	kglt::procedural::mesh::rectangle(mesh_2, 1.0, 1.0);
	
	mesh_1.move_to(-1.0, 1.0, -3.0);
	mesh_2.move_to(1.0, -1.0, -2.0);
	
	kglt::MeshID last_hovered_mesh = 0;
	
	while(window.update()) {
		if(last_hovered_mesh) {
			scene.mesh(last_hovered_mesh).set_diffuse_colour(kglt::Colour(1.0, 1.0, 1.0, 1.0));
		}

		kglt::MeshID hovered_mesh = selection->selected_mesh();
		if(hovered_mesh) {
			scene.mesh(hovered_mesh).set_diffuse_colour(kglt::Colour(1.0, 0.0, 0.0, 1.0));
		} 
		last_hovered_mesh = hovered_mesh;		
	}
    */
	return 0;
}
