#include "deps/stb_truetype/stb_truetype.h"
#include "ttf_loader.h"
#include "../font.h"
#include "../resource_manager.h"

namespace smlt {
namespace loaders {
    void TTFLoader::into(Loadable& resource, const LoaderOptions& options) {
        Font* font = loadable_to<Font>(resource);

        CharacterSet charset = smlt::any_cast<CharacterSet>(options.at("charset"));
        uint32_t font_size = smlt::any_cast<uint32_t>(options.at("size"));

        font->info_.reset(new stbtt_fontinfo());
        font->font_size_ = font_size;

        const std::string buffer_string = this->data_->str();
        const unsigned char* buffer = (const unsigned char*) buffer_string.c_str();
        // Initialize the font data
        stbtt_InitFont(font->info_.get(), buffer, stbtt_GetFontOffsetForIndex(buffer, 0));

        // Generate a new texture for rendering the font to
        font->texture_ = font->resource_manager().new_texture().fetch();
        font->texture_->set_bpp(8); // 8 BPP == GL_RED
        font->texture_->resize(512, 512);

        if(charset != CHARACTER_SET_LATIN) {
            throw std::runtime_error("Unsupported character set - please submit a patch!");
        }

        auto first_char = 32;
        auto char_count = 256 - 32; // Latin-1

        font->char_data_.resize(char_count);

        stbtt_BakeFontBitmap(
            &buffer[0], 0, font_size, &font->texture_->data()[0],
            font->texture_->width(), font->texture_->height(),
            first_char, char_count,
            &font->char_data_[0]
        );

        font->texture_->upload(MIPMAP_GENERATE_COMPLETE, TEXTURE_WRAP_CLAMP_TO_EDGE);
        font->material_ = font->resource_manager().new_material_from_file(Material::BuiltIns::ALPHA_TEXTURE).fetch();
        font->material_->set_texture_unit_on_all_passes(0, font->texture_id());
    }
}
}
