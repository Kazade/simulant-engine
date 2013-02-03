#include "kglt/kglt.h"
#include "kglt/shortcuts.h"

int main(int argc, char* argv[]) {
    logging::get_logger("/")->add_handler(logging::Handler::ptr(new logging::StdIOHandler));
    
    kglt::Window::ptr window = kglt::Window::create();
    window->set_title("KGLT Sample");

    kglt::SubScene& subscene = window->scene().subscene();
    subscene.camera().set_perspective_projection(
        45.0,
        float(window->width()) / float(window->height()),
        1.0,
        1000.0
    );

    /**
        Generate a mesh and build a 2D square

        Base objects are always created with new_X() and can
        be destroyed with delete_X(). They are held by the object
        that spawned them. For example, meshes are held by the scene.

        Creating an object gives you an ID, this can then be exchanged
        for a reference to an object.
    */
    kglt::Mesh& mesh = subscene.mesh(subscene.new_mesh());
    kglt::procedural::mesh::rectangle(mesh, 10.0, 10.0);
	
    ///Shortcut function for loading images
    kglt::TextureID tid = subscene.new_texture();
    kglt::procedural::texture::starfield(subscene.texture(tid));

    kglt::MaterialID matid = kglt::create_material_from_texture(subscene, tid);

	//Apply the texture to the mesh
    mesh.submesh(mesh.submesh_ids()[0]).set_material(matid);

    kglt::Entity& entity = subscene.entity(subscene.new_entity(mesh.id()));

    /**
        Once we have the reference to a base object, we can
        manipulate it easily
    */
    entity.move_to(0.0f, 0.0f, -15.0f);

    //subscene.camera().set_orthographic_projection_from_height(2.0, (float) window->width() / (float)window->height());

    while(window->update()) {
        //entity.rotate_x(2.0 * window->delta_time());
        //entity.rotate_y(1.0 * window->delta_time());
    }
	return 0;
}
