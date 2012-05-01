
#include "shortcuts.h"

namespace kglt {

TextureID create_texture_from_file(Window& window, const std::string& filename) {
	kglt::TextureID tid = window.scene().new_texture();
	kglt::Texture& tex = window.scene().texture(tid);
	window.loader_for(filename)->into(tex);
    tex.upload(true); //Upload
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

}
