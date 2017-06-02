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

        stbtt_fontinfo* info = font->info_.get();

        const std::string buffer_string = this->data_->str();
        const unsigned char* buffer = (const unsigned char*) buffer_string.c_str();
        // Initialize the font data
        stbtt_InitFont(info, buffer, stbtt_GetFontOffsetForIndex(buffer, 0));

        font->scale_ = stbtt_ScaleForPixelHeight(info, (int) font_size);

        int ascent, descent, line_gap;
        stbtt_GetFontVMetrics(info, &ascent, &descent, &line_gap);

        font->ascent_ = float(ascent) * font->scale_;
        font->descent_ = float(descent) * font->scale_;
        font->line_gap_ = float(line_gap) * font->scale_;

        // Generate a new texture for rendering the font to
        font->texture_ = font->resource_manager().new_texture().fetch();
#ifndef _arch_dreamcast
        font->texture_->set_bpp(8); // 8 BPP == GL_RED
#else
        font->texture_->set_bpp(32); // Need to use GL_RGBA on Dreamcast (no GL_RED)
#endif
        font->texture_->resize(512, 512);

        if(charset != CHARACTER_SET_LATIN) {
            throw std::runtime_error("Unsupported character set - please submit a patch!");
        }

        auto first_char = 32;
        auto char_count = 256 - 32; // Latin-1

        font->char_data_.resize(char_count);

#ifndef _arch_dreamcast
        uint8_t* out_buffer = &font->texture_->data()[0];
#else
        // Dreamcast needs 32bpp, so we bake the font bitmap here
        // temporarily and then generate a RGBA texture from it

        std::vector<uint8_t> tmp_buffer(512 * 512);
        uint8_t* out_buffer = &tmp_buffer[0];
#endif
        stbtt_BakeFontBitmap(
            &buffer[0], 0, font_size, out_buffer,
            font->texture_->width(), font->texture_->height(),
            first_char, char_count,
            &font->char_data_[0]
        );

#ifdef _arch_dreamcast
        L_DEBUG("F: Converting font texture from 8bit -> 32bit");

        // Convert from 8bpp to 32bpp
        auto data = &font->texture_->data()[0];
        uint32_t i = 0;
        for(auto& b: tmp_buffer) {
            uint32_t idx = i * 4;
            data[idx] = data[idx + 1] = data[idx + 2] = 255;
            data[idx + 3] = b;
            ++i;
        }
        L_DEBUG("F: Finished conversion");
#endif
        font->texture_->upload(MIPMAP_GENERATE_COMPLETE, TEXTURE_WRAP_CLAMP_TO_EDGE);
        font->texture_->free();
#ifndef _arch_dreamcast
        font->material_ = font->resource_manager().new_material_from_file(Material::BuiltIns::ALPHA_TEXTURE).fetch();
#else
        font->material_ = font->resource_manager().new_material_from_file(Material::BuiltIns::TEXTURE_ONLY).fetch();
#endif
        font->material_->set_texture_unit_on_all_passes(0, font->texture_id());

        L_DEBUG("Font loaded successfully");
    }
}
}
