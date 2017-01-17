#include "font.h"
#include "texture.h"

#define STB_TRUETYPE_IMPLEMENTATION  // force following include to generate implementation
#define STBTT_STATIC

#include "deps/stb_truetype/stb_truetype.h"

namespace smlt {

Font::Font(FontID id, ResourceManager *resource_manager):
    Resource(resource_manager),
    generic::Identifiable<FontID>(id) {

}

TextureID Font::texture_id() const { return texture_->id(); }


}
