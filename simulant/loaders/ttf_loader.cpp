#include "deps/stb_truetype/stb_truetype.h"
#include "ttf_loader.h"
#include "../font.h"
#include "../asset_manager.h"

namespace smlt {
namespace loaders {
    void TTFLoader::into(Loadable& resource, const LoaderOptions& options) {
        const uint32_t TEXTURE_WIDTH = 512;
        const uint32_t TEXTURE_HEIGHT = 512;

        Font* font = loadable_to<Font>(resource);

        CharacterSet charset = smlt::any_cast<CharacterSet>(options.at("charset"));
        uint16_t font_size = smlt::any_cast<uint16_t>(options.at("size"));

        font->info_.reset(new stbtt_fontinfo());
        font->font_size_ = font_size;
        font->page_width_ = TEXTURE_WIDTH;
        font->page_height_ = TEXTURE_HEIGHT;

        stbtt_fontinfo* info = font->info_.get();

        const std::string buffer_string((std::istreambuf_iterator<char>(*this->data_)), std::istreambuf_iterator<char>());
        const unsigned char* buffer = (const unsigned char*) buffer_string.c_str();
        // Initialize the font data
        stbtt_InitFont(info, buffer, stbtt_GetFontOffsetForIndex(buffer, 0));

        font->scale_ = stbtt_ScaleForPixelHeight(info, (int) font_size);

        int ascent, descent, line_gap;
        stbtt_GetFontVMetrics(info, &ascent, &descent, &line_gap);

        font->ascent_ = float(ascent) * font->scale_;
        font->descent_ = float(descent) * font->scale_;
        font->line_gap_ = float(line_gap) * font->scale_;


        if(charset != CHARACTER_SET_LATIN) {
            throw std::runtime_error("Unsupported character set - please submit a patch!");
        }

        auto first_char = 32;
        auto char_count = 256 - 32; // Latin-1

        font->char_data_.resize(char_count);

        // Dreamcast needs 16bpp, so we bake the font bitmap here
        // temporarily and then generate a RGBA texture from it

        std::vector<uint8_t> tmp_buffer(TEXTURE_WIDTH * TEXTURE_HEIGHT);
        uint8_t* out_buffer = &tmp_buffer[0];
        stbtt_BakeFontBitmap(
            &buffer[0], 0, font_size, out_buffer,
            TEXTURE_WIDTH, TEXTURE_HEIGHT,
            first_char, char_count,
            (stbtt_bakedchar*) &font->char_data_[0]
        );

        /* We create a 16 colour paletted texture. This is an RGBA texture
         * where the colour is white with 16 levels of alpha. We then pack
         * into 4bpp data. This means that a 512x512 texture takes up less than
         * 150kb - essential for memory constrained systems. */

        S_DEBUG("Converting to paletted format");
        std::vector<uint8_t> palette_data;
        for(int i = 0; i < 16; ++i) {
            palette_data.push_back(255);
            palette_data.push_back(255);
            palette_data.push_back(255);
            palette_data.push_back((i * 17));
        }

        for(std::size_t i = 0; i < tmp_buffer.size(); i += 2) {
            uint8_t t0 = tmp_buffer[i];
            uint8_t t1 = tmp_buffer[i + 1];
            uint8_t i0 = t0 >> 4;
            uint8_t i1 = t1 >> 4;
            palette_data.push_back((i0 << 4) | i1);
        }

        S_DEBUG("Finished conversion");

        // Generate a new texture for rendering the font to
        auto texture = font->texture_ = font->asset_manager().new_texture(
            TEXTURE_WIDTH,
            TEXTURE_HEIGHT,
            TEXTURE_FORMAT_RGBA8_PALETTED4
        );

#if defined(__DREAMCAST__)
        /* FIXME: Implement 4bpp mipmap generation in GLdc */
        texture->set_mipmap_generation(MIPMAP_GENERATE_NONE);
#endif

        texture->set_texture_filter(TEXTURE_FILTER_BILINEAR);
        texture->set_data(palette_data);
        texture->set_free_data_mode(TEXTURE_FREE_DATA_AFTER_UPLOAD);
        texture->flush();

        font->material_ = font->asset_manager().new_material_from_file(Material::BuiltIns::TEXTURE_ONLY);
        font->material_->set_diffuse_map(font->texture_);

        font->material_->set_blend_func(BLEND_ALPHA);
        font->material_->set_depth_test_enabled(false);
        font->material_->set_cull_mode(CULL_MODE_NONE);

        S_DEBUG("Font loaded successfully");
    }
}
}
