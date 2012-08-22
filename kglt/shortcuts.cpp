#include "scene.h"
#include "shortcuts.h"

namespace kglt {

TextureID create_texture_from_file(WindowBase& window, const std::string& filename, bool upload) {
	kglt::TextureID tid = window.scene().new_texture();
	kglt::Texture& tex = window.scene().texture(tid);
	window.loader_for(filename)->into(tex);
	
    if(upload) {
        //Upload the texture in the main thread, regardless of where this was called from
        window.idle().add_once(sigc::bind(sigc::mem_fun(&tex, &kglt::Texture::upload), true, true, true));
    }

    return tid;
}

Mesh& return_new_mesh(Scene& scene) {
	MeshID mid = scene.new_mesh();
	return scene.mesh(mid);
}

ShaderProgram& return_new_shader(Scene& scene) {
	ShaderID sid = scene.new_shader();
	return scene.shader(sid);
}

Text& return_new_text(Scene &scene) {
    TextID tid = scene.new_text();
    return scene.text(tid);
}

Texture& return_new_texture(Scene& scene) {
    TextureID tid = scene.new_texture();
    return scene.texture(tid);
}

Material& return_new_material(Scene& scene) {
    MaterialID mid = scene.new_material();
    return scene.material(mid);
}

MaterialID create_material_from_texture(Scene& scene, TextureID tex) {
    kglt::MaterialID matid = scene.new_material();
    kglt::Material& mat = scene.material(matid);
    mat.technique().pass(mat.technique().new_pass(scene.default_shader())).set_texture_unit(0, tex);
    return matid;
}

}
