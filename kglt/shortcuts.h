#ifndef SHORTCUTS_H_INCLUDED
#define SHORTCUTS_H_INCLUDED

#include <string>
#include "window.h"
#include "types.h"

namespace kglt {

TextureID create_texture_from_file(WindowBase& window, const std::string& filename);

Mesh& return_new_mesh(Scene& scene);
Sprite& return_new_sprite(Scene& scene);
ShaderProgram& return_new_shader(Scene& scene);
Text& return_new_text(Scene& scene);

MaterialID create_material_from_texture(Scene& scene, TextureID tex);

}

#endif // SHORTCUTS_H_INCLUDED
