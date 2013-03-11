#include "scene.h"
#include "shortcuts.h"

namespace kglt {

TextureID create_texture_from_file(ResourceManager& rm, const std::string& filename, bool upload) {
    kglt::TextureID tid = rm.new_texture();
    kglt::Texture& tex = rm.texture(tid);
    rm.window().loader_for(filename)->into(tex);

    if(upload) {
        //Upload the texture in the main thread, regardless of where this was called from
        rm.window().idle().add_once(sigc::bind(sigc::mem_fun(&tex, &kglt::Texture::upload), true, true, true, false));
    }

    return tid;
}

Mesh& return_new_mesh(ResourceManager& scene) {
    MeshID mid = scene.new_mesh();
    return scene.mesh(mid);
}

ShaderProgram& return_new_shader(ResourceManager& scene) {
    ShaderID sid = scene.new_shader();
    return scene.shader(sid);
}

Texture& return_new_texture(ResourceManager& scene) {
    TextureID tid = scene.new_texture();
    return scene.texture(tid);
}

Material& return_new_material(ResourceManager& scene) {
    MaterialID mid = scene.new_material();
    return scene.material(mid);
}

MaterialID create_material_from_texture(ResourceManager& scene, TextureID tex) {
    //Duplicate the default material
    kglt::Material& mat = scene.material(scene.new_material());
    scene.window().loader_for("kglt/materials/generic_multitexture.kglm")->into(mat);

    //Set texture unit 0 to this texture
    mat.technique().pass(0).set_texture_unit(0, tex);
    return mat.id();
}

}
