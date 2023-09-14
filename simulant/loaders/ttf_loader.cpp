#include "deps/stb_rect_pack/stb_rect_pack.h"
#include "deps/stb_truetype/stb_truetype.h"
#include "ttf_loader.h"
#include "../font.h"
#include "../asset_manager.h"
#include "../platform.h"

namespace smlt {
namespace loaders {
    void TTFLoader::into(Loadable& resource, const LoaderOptions& options) {
        Font* font = loadable_to<Font>(resource);

        CharacterSet charset = smlt::any_cast<CharacterSet>(options.at("charset"));
        uint16_t font_size = smlt::any_cast<uint16_t>(options.at("size"));
        std::size_t blur = smlt::any_cast<std::size_t>(options.at("blur_radius"));

        font->info_.reset(new stbtt_fontinfo());
        font->font_size_ = font_size;

        stbtt_fontinfo* info = font->info_.get();

        data_->seekg(0, std::ios::end);
        auto e = data_->tellg();
        data_->seekg(0, std::ios::beg);

        std::vector<char> data(e);
        data_->read(data.data(), data.size());

        unsigned char* buffer = (unsigned char*) &data[0];
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

        stbtt_pack_range blocks[2];
        blocks[0].font_size = blocks[1].font_size = font_size;
        blocks[0].array_of_unicode_codepoints = blocks[1].array_of_unicode_codepoints = 0;

        blocks[0].first_unicode_codepoint_in_range = 0x20;
        blocks[0].num_chars = 0x7E - 0x20;

        blocks[1].first_unicode_codepoint_in_range = 0xA0;
        blocks[1].num_chars = 0xFF - 0xA0;

        size_t char_count = blocks[0].num_chars + blocks[1].num_chars;
        std::vector<stbtt_packedchar> char_data(char_count);

        blocks[0].chardata_for_range = &char_data[0];
        blocks[1].chardata_for_range = &char_data[blocks[0].num_chars];

        int width = 256;
        std::vector<uint8_t> pixels;
        while(width < 2048) {
            pixels.resize(width * width);

            stbtt_pack_context ctx;
            stbtt_PackBegin(&ctx, &pixels[0], width, width, 0, 1, NULL);

            int ret = stbtt_PackFontRanges(&ctx, buffer, 0, blocks, 2);

            stbtt_PackEnd(&ctx);

            if(ret == 1) {
                S_INFO("Packed font into {0}x{0} texture", width);
                break;
            }

            width <<= 1;
        }

        if(width == 2048) {
            S_ERROR("Failed to pack font successfully. Characters too large");
            return;
        }

        font->page_width_ = width;
        font->page_height_ = width;

        for(auto& block: blocks) {
            for(size_t i = 0; i < block.num_chars; ++i) {
                stbtt_aligned_quad q;
                char32_t codepoint = block.first_unicode_codepoint_in_range + i;
                float dummy_x = 0, dummy_y = 0;
                const stbtt_packedchar *pc = &block.chardata_for_range[i];
                stbtt_GetPackedQuad(block.chardata_for_range, width, width, i, &dummy_x, &dummy_y, &q, 1);

                bool invert = true;
                if(invert) {
                    std::swap(q.t0, q.t1);
                    std::swap(q.y0, q.y1);
                    q.y0 *= -1;
                    q.y1 *= -1;
                    q.y0 -= font->ascent();
                    q.y1 -= font->ascent();
                }

                font->char_data_[codepoint].xy0.x = q.x0;
                font->char_data_[codepoint].xy1.x = q.x1;
                font->char_data_[codepoint].xy0.y = q.y0;
                font->char_data_[codepoint].xy1.y = q.y1;

                font->char_data_[codepoint].st0.x = q.s0;
                font->char_data_[codepoint].st1.x = q.s1;
                font->char_data_[codepoint].st0.y = q.t0;
                font->char_data_[codepoint].st1.y = q.t1;

                font->char_data_[codepoint].xadvance = pc->xadvance;
            }
        }

        char_data.clear();
        char_data.shrink_to_fit();

        // Dreamcast needs 16bpp, so we bake the font bitmap here
        // temporarily and then generate a RGBA texture from it

        auto tmp_texture = font->asset_manager().new_texture(
            width,
            width,
            TEXTURE_FORMAT_R_1UB_8
        );

        tmp_texture->set_auto_upload(false);
        tmp_texture->set_data(std::move(pixels));

        /* We don't need the file data anymore */
        data.clear();
        data.shrink_to_fit();

        if(blur) {
            tmp_texture->blur(BLUR_TYPE_SIMPLE, blur);
        }

        /* We create a 16 colour paletted texture. This is an RGBA texture
         * where the colour is white with 16 levels of alpha. We then pack
         * into 4bpp data. This means that a 512x512 texture takes up less than
         * 150kb - essential for memory constrained systems. */

        S_DEBUG("Converting to paletted format");

        const uint8_t* tmp_buffer = tmp_texture->data();

        // Generate a new texture for rendering the font to
        auto texture = font->texture_ = font->asset_manager().new_texture(
            width,
            width,
            TEXTURE_FORMAT_RGBA8_PALETTED4
        );

#if defined(__DREAMCAST__)
        /* FIXME: Implement 4bpp mipmap generation in GLdc */
        texture->set_mipmap_generation(MIPMAP_GENERATE_NONE);
#endif
        texture->set_texture_filter(TEXTURE_FILTER_BILINEAR);
        texture->set_free_data_mode(TEXTURE_FREE_DATA_AFTER_UPLOAD);
        texture->mutate_data([&](uint8_t* palette_data, uint16_t, uint16_t, TextureFormat) {
            uint8_t* pout = palette_data;
            for(int i = 0; i < 16; ++i) {
                *(pout++) = 255;
                *(pout++) = 255;
                *(pout++) = 255;
                *(pout++) = (i * 17);
            }

            for(std::size_t i = 0; i < tmp_texture->data_size(); i += 2) {
                uint8_t t0 = tmp_buffer[i];
                uint8_t t1 = tmp_buffer[i + 1];
                uint8_t i0 = t0 >> 4;
                uint8_t i1 = t1 >> 4;
                *(pout++) = ((i0 << 4) | i1);
            }
        });

        /* We're done with this now */
        tmp_texture.reset();
        S_DEBUG("Finished conversion");

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
