#include "scene.h"
#include "shortcuts.h"
#include "loader.h"

namespace kglt {

MaterialID create_material_from_texture(ResourceManager& scene, TextureID tex) {


    //Duplicate the default material        
    auto mat = scene.material(scene.scene().clone_default_material());

    //Set texture unit 0 to this texture
    mat->technique().pass(0).set_texture_unit(0, tex);
    return mat->id();
}

}
