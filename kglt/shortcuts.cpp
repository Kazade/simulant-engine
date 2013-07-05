#include "scene.h"
#include "shortcuts.h"
#include "loader.h"

namespace kglt {

TextureID create_texture_from_file(ResourceManager& rm, const std::string& filename, bool upload) {
    auto tex = rm.texture(rm.new_texture());

    rm.window().loader_for(filename)->into(*tex);

    if(upload) {
        tex->upload(true, true, true, false);
    }

    //Release ownership of this so it doesn't get GC'd
    rm.mark_texture_as_uncollected(tex->id());
    return tex->id();
}

MaterialID create_material_from_texture(ResourceManager& scene, TextureID tex) {


    //Duplicate the default material        
    auto mat = scene.material(scene.scene().clone_default_material());

    //Set texture unit 0 to this texture
    mat->technique().pass(0).set_texture_unit(0, tex);
    return mat->id();
}

}
