#include "GL/window.h"
#include "GL/types.h"

int main(int argc, char* argv[]) {
	GL::Window window;
	window.set_title("KGLT Sample");

	window.scene().render_options.backface_culling_enabled = false;

	GL::MeshID mid = window.scene().new_mesh();

	window.scene().mesh(mid).move(0.0f, 0.0f, -4.0f);
	window.scene().mesh(mid).add_vertex(-1.0, -1.0f, 0.0f);
	window.scene().mesh(mid).add_vertex(1.0f, -1.0f, 0.0f);
	window.scene().mesh(mid).add_vertex(1.0f, 1.0f, 0.0f);
	window.scene().mesh(mid).add_vertex(-1.0f, 1.0f, 0.0f);
	window.scene().mesh(mid).add_triangle(0, 1, 2);
	window.scene().mesh(mid).add_triangle(0, 2, 3);

	//Create a texture, use the TGA loader to fill it
	GL::TextureID tid = window.scene().new_texture();
	GL::Texture& tex = window.scene().texture(tid);
	window.loader("tga").load_into(tex, "sample.tga");

	//Apply the texture to the mesh
	window.scene().mesh(mid).apply_texture(0, tid);

	while(window.update()) {}
	return 0;
}
