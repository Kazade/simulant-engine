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

        std::vector<stbtt_pack_range> blocks;
        std::vector<stbtt_packedchar> char_data;
        stbtt_pack_context ctx;

        std::size_t packed = 0;
        int current_page = 0;

        std::map<int, int> blocks_to_pages;

        auto upload_texture = [&](int width, std::vector<uint8_t>&& pixels) {
            // Dreamcast needs 16bpp, so we bake the font bitmap here
            // temporarily and then generate a RGBA texture from it

            auto tmp_texture = font->asset_manager().create_texture(
                width,
                width,
                TEXTURE_FORMAT_R_1UB_8
            );

            tmp_texture->set_auto_upload(false);
            tmp_texture->set_data(std::move(pixels));

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
            auto texture = font->pages_[current_page].texture = font->asset_manager().create_texture(
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

            font->pages_[current_page].material = font->asset_manager().load_material(Material::BuiltIns::TEXTURE_ONLY);
            font->pages_[current_page].material->set_diffuse_map(font->pages_[current_page].texture);

            font->pages_[current_page].material->set_blend_func(BLEND_ALPHA);
            font->pages_[current_page].material->set_depth_test_enabled(false);
            font->pages_[current_page].material->set_cull_mode(CULL_MODE_NONE);
        };

        auto push_block = [&](int first_codepoint, int num_chars) {
            stbtt_pack_range new_block;
            new_block.font_size = font_size;
            new_block.array_of_unicode_codepoints = 0;
            new_block.first_unicode_codepoint_in_range = first_codepoint;
            new_block.num_chars = num_chars;
            blocks.push_back(new_block);
        };

        auto split_block = [&]() -> bool {
            stbtt_pack_range* largest = nullptr;

            // Find the largest block
            for(std::size_t i = packed; i < blocks.size(); ++i) {
                if(!largest || blocks[i].num_chars > largest->num_chars) {
                    largest = &blocks[i];
                }
            }

            assert(largest);

            if(largest->num_chars == 1) {
                S_ERROR("Couldn't split block any further");
                return false;
            }

            stbtt_pack_range new_block;
            new_block.font_size = largest->font_size;
            new_block.array_of_unicode_codepoints = 0;
            new_block.first_unicode_codepoint_in_range = largest->first_unicode_codepoint_in_range + (largest->num_chars / 2);
            new_block.num_chars = largest->num_chars / 2;
            new_block.chardata_for_range = largest->chardata_for_range + (largest->num_chars / 2);

            blocks.push_back(new_block);

            largest->num_chars /= 2;

            return true;
        };

        // Initial blocks for LATIN-1 support
        push_block(0x20, 0x7E - 0x20);
        push_block(0xA0, 0xFF - 0xA0);

        std::size_t char_count = 0;
        for(auto& block: blocks) {
            char_count += block.num_chars;
        }

        char_data.resize(char_count);

        std::size_t offset = 0;
        for(auto& block: blocks) {
            block.chardata_for_range = &char_data[offset];
            offset += block.num_chars;
        }

        while(packed < blocks.size()) {
            // Phase 1. Increment texture size
            font->pages_.push_back(FontPage());

            int width = 256;
            int blocks_to_pack = blocks.size() - packed;

            std::vector<uint8_t> pixels;
            while(width < 2048) {
                pixels.resize(width * width);

                stbtt_PackBegin(&ctx, &pixels[0], width, width, 0, 1, NULL);

                int ret = stbtt_PackFontRanges(&ctx, buffer, 0, &blocks[0] + packed, blocks_to_pack);

                stbtt_PackEnd(&ctx);

                if(ret == 1) {
                    S_INFO("Packed font into {0}x{0} texture", width);
                    break;
                }

                width <<= 1;
            }

            // Phase 2. Reduce block sizes
            if(width == 2048) {
                width = 1024;
                pixels.resize(width * width);

                while(blocks_to_pack > 1) {
                    // We failed to pack into 1024, so now we need to break the
                    // blocks down until things fit
                    if(!split_block()) {
                        // If we can't break down the blocks any further, we
                        // reduce the number of blocks we're trying to pack
                        blocks_to_pack--;
                    }

                    stbtt_PackBegin(&ctx, &pixels[0], width, width, 0, 1, NULL);

                    int ret = stbtt_PackFontRanges(&ctx, buffer, 0, &blocks[0] + packed, blocks_to_pack);

                    stbtt_PackEnd(&ctx);

                    if(ret == 1) {
                        S_INFO("Packed {0} blocks into texture {1}", blocks_to_pack, current_page);
                        upload_texture(width, std::move(pixels));

                        for(int i = 0; i < blocks_to_pack; ++i) {
                            blocks_to_pages[packed + i] = current_page;
                        }

                        packed += blocks_to_pack;
                        blocks_to_pack = 0;
                        font->pages_[current_page].width = width;
                        font->pages_[current_page].height = width;
                        break;
                    }
                }

                current_page++;  // Move to the next page for the remaining blocks
            } else {
                upload_texture(width, std::move(pixels));

                // Store the mapping of blocks to pages
                for(std::size_t i = packed; i < blocks.size(); ++i) {
                    blocks_to_pages[i] = current_page;
                }

                packed = blocks.size();
                font->pages_[current_page].width = width;
                font->pages_[current_page].height = width;
            }
        }

        std::size_t block_num = 0;
        for(auto& block: blocks) {
            for(int i = 0; i < block.num_chars; ++i) {
                stbtt_aligned_quad q;
                char32_t codepoint = block.first_unicode_codepoint_in_range + i;
                float dummy_x = 0, dummy_y = 0;
                const stbtt_packedchar *pc = &block.chardata_for_range[i];

                int width = font->pages_[blocks_to_pages[block_num]].width;

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

                font->char_data_[codepoint].page = blocks_to_pages[block_num];
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

            ++block_num;
        }

        char_data.clear();
        char_data.shrink_to_fit();

        /* We don't need the file data anymore */
        data.clear();
        data.shrink_to_fit();


        S_DEBUG("Font loaded successfully");
    }
}
}
