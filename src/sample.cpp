#include "GL/window.h"
#include "GL/types.h"
#include "GL/shortcuts.h"

int main(int argc, char* argv[]) {
	GL::Window window;
	window.set_title("KGLT Sample");

    /**
        Generate a mesh and build a 2D square

        Base objects are always created with new_X() and can
        be destroyed with delete_X(). They are held by the object
        that spawned them. For example, meshes are held by the scene.

        Creating an object gives you an ID, this can then be exchanged
        for a reference to an object.
    */
    GL::MeshID mid = window.scene().new_mesh();
    GL::Mesh& mesh = window.scene().mesh(mid);

    /**
        Once we have the reference to a base object, we can
        manipulate it easily
    */
	mesh.move(0.0f, 0.0f, -4.0f);
	mesh.add_vertex(-1.0, -1.0f, 0.0f, 0.0f, 0.0f);
	mesh.add_vertex(1.0f, -1.0f, 0.0f, 1.0f, 0.0f);
	mesh.add_vertex(1.0f, 1.0f, 0.0f, 1.0f, 1.0f);
	mesh.add_vertex(-1.0f, 1.0f, 0.0f, 0.0f, 1.0f);
	mesh.add_triangle(0, 1, 2);
	mesh.add_triangle(0, 2, 3);

    ///Shortcut function for loading images
    GL::TextureID tid = GL::create_texture_from_file(window, "sample.tga");
	//Apply the texture to the mesh
	mesh.apply_texture(0, tid);

	while(window.update()) {}
	return 0;
}
