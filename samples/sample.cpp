#include "kglt/kglt.h"
#include "kglt/shortcuts.h"

int main(int argc, char* argv[]) {
    logging::get_logger("/")->add_handler(logging::Handler::ptr(new logging::StdIOHandler));
    
    kglt::Window::ptr window = kglt::Window::create();
    window->set_title("KGLT Sample");

    kglt::Scene& scene = window->scene();

    /**
        Generate a mesh and build a 2D square

        Base objects are always created with new_X() and can
        be destroyed with delete_X(). They are held by the object
        that spawned them. For example, meshes are held by the scene.

        Creating an object gives you an ID, this can then be exchanged
        for a reference to an object.
    */
    kglt::Mesh& mesh = scene.mesh(scene.new_mesh());
    kglt::procedural::mesh::rectangle(mesh, 1.0, 1.0);
	
    ///Shortcut function for loading images
    kglt::TextureID tid = kglt::create_texture_from_file(*window, "sample.tga");
    kglt::MaterialID matid = kglt::create_material_from_texture(scene, tid);

	//Apply the texture to the mesh
    mesh.submesh(mesh.submesh_ids()[0]).set_material(matid);

    kglt::Entity& entity = scene.entity(scene.new_entity(mesh.id()));

    /**
        Once we have the reference to a base object, we can
        manipulate it easily
    */
    entity.move_to(0.0f, 0.0f, -5.0f);

    scene.camera().set_orthographic_projection_from_height(2.0, (float) window->width() / (float)window->height());

    while(window->update()) {}
	return 0;
}
