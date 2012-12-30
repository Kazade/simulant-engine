#ifndef SHORTCUTS_H_INCLUDED
#define SHORTCUTS_H_INCLUDED

#include <string>
#include "window.h"
#include "types.h"
#include "shader.h"
#include "texture.h"
#include "material.h"

namespace kglt {

TextureID create_texture_from_file(ResourceManager& rm, const std::string& filename, bool upload=true);

Mesh& return_new_mesh(ResourceManager& scene);
ShaderProgram& return_new_shader(ResourceManager& scene);
Texture& return_new_texture(ResourceManager& scene);
Material& return_new_material(ResourceManager& scene);
MaterialID create_material_from_texture(ResourceManager& scene, TextureID tex);

}

#endif // SHORTCUTS_H_INCLUDED
