
#include "shortcuts.h"

namespace GL {

TextureID create_texture_from_file(Window& window, const std::string& filename) {
	GL::TextureID tid = window.scene().new_texture();
	GL::Texture& tex = window.scene().texture(tid);
	window.loader_for(filename)->into(tex);
    tex.upload(true); //Upload
    return tid;
}

}
