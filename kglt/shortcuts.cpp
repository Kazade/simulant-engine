
#include "shortcuts.h"

namespace kglt {

TextureID create_texture_from_file(WindowBase& window, const std::string& filename) {
	kglt::TextureID tid = window.scene().new_texture();
	kglt::Texture& tex = window.scene().texture(tid);
	window.loader_for(filename)->into(tex);
	
	//Upload the texture in the main thread, regardless of where this was called from
	window.idle().add_once(sigc::bind(sigc::mem_fun(&tex, &kglt::Texture::upload), true, true, true));
    return tid;
}

Mesh& return_new_mesh(Scene& scene) {
	MeshID mid = scene.new_mesh();
	return scene.mesh(mid);
}

Sprite& return_new_sprite(Scene& scene) {
	SpriteID sid = scene.new_sprite();
	return scene.sprite(sid);
}

ShaderProgram& return_new_shader(Scene& scene) {
	ShaderID sid = scene.new_shader();
	return scene.shader(sid);
}

}
