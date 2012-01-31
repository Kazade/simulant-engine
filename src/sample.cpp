#include "kglt/window.h"
#include "kglt/types.h"
#include "kglt/shortcuts.h"
#include "kglt/mesh.h"

int main(int argc, char* argv[]) {
    logging::get_logger("/")->add_handler(logging::Handler::ptr(new logging::StdIOHandler));
    
	kglt::Window window;
	window.set_title("KGLT Sample");

    /**
        Generate a mesh and build a 2D square

        Base objects are always created with new_X() and can
        be destroyed with delete_X(). They are held by the object
        that spawned them. For example, meshes are held by the scene.

        Creating an object gives you an ID, this can then be exchanged
        for a reference to an object.
    */
    kglt::MeshID mid = window.scene().new_mesh();
    kglt::Mesh& mesh = window.scene().mesh(mid);

    window.scene().render_options.backface_culling_enabled = false;
    /**
        Once we have the reference to a base object, we can
        manipulate it easily
    */
	mesh.move_to(0.0f, 0.0f, -5.0f);
	mesh.add_vertex(-1.0, -1.0f, 0.0f);
	mesh.add_vertex(1.0f, -1.0f, 0.0f);
	mesh.add_vertex(1.0f, 1.0f, 0.0f);
	mesh.add_vertex(-1.0f, 1.0f, 0.0f);
	kglt::Triangle& tri1 = mesh.add_triangle(0, 1, 2);
	tri1.uv[0].x = 0.0f; tri1.uv[0].y = 0.0f;
	tri1.uv[1].x = 1.0f; tri1.uv[1].y = 0.0f;
	tri1.uv[2].x = 1.0f; tri1.uv[2].y = 1.0f;
	
	kglt::Triangle& tri2 = mesh.add_triangle(0, 2, 3);
	tri2.uv[0].x = 0.0f; tri2.uv[0].y = 0.0f;
	tri2.uv[1].x = 1.0f; tri2.uv[1].y = 1.0f;
	tri2.uv[2].x = 0.0f; tri2.uv[2].y = 1.0f;
	
    ///Shortcut function for loading images
    kglt::TextureID tid = kglt::create_texture_from_file(window, "sample.tga");
	//Apply the texture to the mesh
	mesh.apply_texture(tid, kglt::TextureLevel::PRIMARY);

	while(window.update()) {}
	return 0;
}
