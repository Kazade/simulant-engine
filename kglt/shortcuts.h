#ifndef SHORTCUTS_H_INCLUDED
#define SHORTCUTS_H_INCLUDED

#include <string>
#include "window.h"
#include "types.h"

namespace kglt {

TextureID create_texture_from_file(Window& scene, const std::string& filename);

}

#endif // SHORTCUTS_H_INCLUDED
