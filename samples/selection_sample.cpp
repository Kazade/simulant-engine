#include "simulant/simulant.h"
#include "simulant/shortcuts.h"

int main(int argc, char* argv[]) {
    _S_UNUSED(argc);
    _S_UNUSED(argv);

	//Set up logging to stdio

/** FIXME:
 *  THIS SAMPLE IS TOTALLY BROKEN AS IT SHOULD BE IMPLEMENTED
 *  IN TERMS OF PASSES RATHER THAN A SEPARATE RENDERER
 *
	logging::get_logger("main")->add_handler(logging::Handler::ptr(new logging::StdIOHandler));
		
	smlt::Window window;	
	smlt::Scene& scene = window.scene();
	
	window.show_cursor(true); //Show the cursor
	
	scene.remove_all_passes();		
	
    auto selection = smlt::SelectionRenderer::create();
    scene.add_pass(selection); //Add the selection pass
    scene.add_pass(smlt::GenericRenderer::create()); //Normal rendering
	
    scene.active_camera().set_perspective_projection(
        45.0,
        float(window.width()) / float(window.height()),
        0.1,
        1000.0
    );

	smlt::Mesh& mesh_1 = smlt::return_new_mesh(scene);
	smlt::Mesh& mesh_2 = smlt::return_new_mesh(scene);
	
	smlt::procedural::mesh::rectangle(mesh_1, 1.0, 1.0);
	smlt::procedural::mesh::rectangle(mesh_2, 1.0, 1.0);
	
	mesh_1.move_to(-1.0, 1.0, -3.0);
	mesh_2.move_to(1.0, -1.0, -2.0);
	
	smlt::MeshID last_hovered_mesh = 0;
	
	while(window.update()) {
		if(last_hovered_mesh) {
			scene.mesh(last_hovered_mesh).set_diffuse_colour(smlt::Colour(1.0, 1.0, 1.0, 1.0));
		}

		smlt::MeshID hovered_mesh = selection->selected_mesh();
		if(hovered_mesh) {
			scene.mesh(hovered_mesh).set_diffuse_colour(smlt::Colour(1.0, 0.0, 0.0, 1.0));
		} 
		last_hovered_mesh = hovered_mesh;		
	}
    */
	return 0;
}
