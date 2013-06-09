#include "scene.h"
#include "shortcuts.h"
#include "loader.h"

namespace kglt {

TextureID create_texture_from_file(ResourceManager& rm, const std::string& filename, bool upload) {
    auto tex = rm.texture(rm.new_texture());
    TextureID tex_id = tex->id();

    rm.window().loader_for(filename)->into(*tex);

    if(upload) {
        //Upload the texture in the main thread, regardless of where this was called from
        rm.window().idle().add_once([&] () {
            auto tex = rm.texture(tex_id);
            tex->upload(true, true, true, false);
        });
    }

    return tex->id();
}

MaterialID create_material_from_texture(ResourceManager& scene, TextureID tex) {
    //Duplicate the default material
    auto mat = scene.material(scene.new_material());

    scene.window().loader_for("kglt/materials/multitexture_and_lighting.kglm")->into(*mat.__object);

    //Set texture unit 0 to this texture
    mat->technique().pass(0).set_texture_unit(0, tex);
    return mat->id();
}

}
