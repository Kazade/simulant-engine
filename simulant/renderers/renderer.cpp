//
//   Copyright (c) 2011-2017 Luke Benstead https://simulant-engine.appspot.com
//
//     This file is part of Simulant.
//
//     Simulant is free software: you can redistribute it and/or modify
//     it under the terms of the GNU Lesser General Public License as published by
//     the Free Software Foundation, either version 3 of the License, or
//     (at your option) any later version.
//
//     Simulant is distributed in the hope that it will be useful,
//     but WITHOUT ANY WARRANTY; without even the implied warranty of
//     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//     GNU Lesser General Public License for more details.
//
//     You should have received a copy of the GNU Lesser General Public License
//     along with Simulant.  If not, see <http://www.gnu.org/licenses/>.
//

#include "renderer.h"
#include "../texture.h"

namespace smlt {

void Renderer::register_texture(AssetID tex_id, Texture* texture) {
    on_texture_register(tex_id, texture);
    texture_registry_.insert(std::make_pair(tex_id, texture));
}

void Renderer::unregister_texture(AssetID texture_id, Texture* texture) {
    texture_registry_.erase(texture_id);
    on_texture_unregister(texture_id, texture);
}

bool Renderer::texture_format_is_native(TextureFormat fmt) {
    switch(fmt) {
        case TEXTURE_FORMAT_R_1UB_8:
        case TEXTURE_FORMAT_RGB_3UB_888:
        case TEXTURE_FORMAT_RGBA_4UB_8888:
        case TEXTURE_FORMAT_RGB_1US_565:
        case TEXTURE_FORMAT_ARGB_1US_4444:
        case TEXTURE_FORMAT_RGBA_1US_4444:
        case TEXTURE_FORMAT_ARGB_1US_1555:
        case TEXTURE_FORMAT_RGBA_1US_5551:

#ifdef __DREAMCAST__
        case TEXTURE_FORMAT_RGB_1US_565_TWID:
        case TEXTURE_FORMAT_ARGB_1US_4444_TWID:
        case TEXTURE_FORMAT_ARGB_1US_1555_TWID:
        case TEXTURE_FORMAT_RGB_1US_565_VQ_TWID:
        case TEXTURE_FORMAT_ARGB_1US_4444_VQ_TWID:
        case TEXTURE_FORMAT_ARGB_1US_1555_VQ_TWID:
        case TEXTURE_FORMAT_RGB_1US_565_VQ_TWID_MIP:
        case TEXTURE_FORMAT_ARGB_1US_4444_VQ_TWID_MIP:
        case TEXTURE_FORMAT_ARGB_1US_1555_VQ_TWID_MIP:
#endif
            return true;
        default:
            return false;
    }
}

bool Renderer::texture_format_is_usable(TextureFormat fmt) {
    switch(fmt) {
        /* All these formats can be converted to more
         * natively supported formats */
        case TEXTURE_FORMAT_ARGB_1US_1555_TWID:
        case TEXTURE_FORMAT_ARGB_1US_1555_VQ_TWID:
        case TEXTURE_FORMAT_ARGB_1US_1555_VQ_TWID_MIP:
        case TEXTURE_FORMAT_ARGB_1US_4444_TWID:
        case TEXTURE_FORMAT_ARGB_1US_4444_VQ_TWID:
        case TEXTURE_FORMAT_ARGB_1US_4444_VQ_TWID_MIP:
        case TEXTURE_FORMAT_RGB_1US_565_TWID:
        case TEXTURE_FORMAT_RGB_1US_565_VQ_TWID:
        case TEXTURE_FORMAT_RGB_1US_565_VQ_TWID_MIP:
        /* These formats can be converted to 16-bit PVR formats */
        case TEXTURE_FORMAT_RGBA_4UB_8888:
        case TEXTURE_FORMAT_RGB_3UB_888:
        case TEXTURE_FORMAT_R_1UB_8:
        /* Paletted formats - need to be expanded to real color data */
        case TEXTURE_FORMAT_RGB8_PALETTED4:
        case TEXTURE_FORMAT_RGBA8_PALETTED4:
        case TEXTURE_FORMAT_RGB565_PALETTED4:
        case TEXTURE_FORMAT_RGB8_PALETTED8:
        case TEXTURE_FORMAT_RGBA8_PALETTED8:
        case TEXTURE_FORMAT_RGB565_PALETTED8:
            return true;
        default:
            return false;
    }
}

bool Renderer::is_texture_registered(AssetID texture_id) const {
    return texture_registry_.count(texture_id);
}

void Renderer::pre_render() {
    on_pre_render();

    for(auto& wptr: texture_registry_) {
        prepare_texture(wptr.second);
    }
}

void Renderer::post_render() {
    on_post_render();
}

static bool format_in_list(TextureFormat fmt, const TextureFormat* values) {
    while(*values != 0) {
        if(*values == fmt) {
            return true;
        }
        values++;
    }
    return false;
}

/* Details: https://dcemulation.org/phpBB/viewtopic.php?p=748490&sid=d94a7855fd2ff6a616b0c7a8c10e2e09#p748490
 *
 * But mainly:
 *
 * - "2048 bytes of codebook, followed by one byte of padding, and then the VQ compressed data"
 * - "For mip maps, it appears that the mip levels are stored directly after the main texture,
 * in decreasing order of size. Each mip level is half the size (width and height) of the previous one,
 * as with just about every other implementation of mip mapping. Every possible mip level, down to 2x2, is used."
 * - The texture data then consists of an 8 bit index per 2x2 pixel group, arranged in the usual UV Twiddled (technically "Morton") order.
 */
static void decompress_16bpp(const uint8_t* src, uint8_t* dst, uint32_t w, uint32_t h) {
    auto get_untwiddled_index = [](uint16_t w, uint16_t h, uint32_t p) -> uint32_t {
        uint32_t ddx = 1, ddy = w;
        uint32_t q = 0;

        for(int i = 0; i < 16; i++){
            if(h >>= 1){
                if(p & 1){q |= ddy;}
                p >>= 1;
            }
            ddy <<= 1;
            if(w >>= 1){
                if(p & 1){q |= ddx;}
                p >>= 1;
            }
            ddx <<= 1;
        }

        return q;
    };

    const uint64_t* codebook = (uint64_t*) src;
    src += 2048; /* 2kb of the data is the codebook */

    const uint32_t pixels = (w / 2) * (h / 2);
    uint16_t* output = (uint16_t*) dst;

    uint32_t hw = w / 2;

    for(uint32_t i = 0; i < pixels; ++i, ++src) {
        uint64_t codebook_entry = codebook[*src];

        uint16_t t0 = ((codebook_entry) & 0xFFFF000000000000) >> 48;
        uint16_t t1 = ((codebook_entry) & 0x0000FFFF00000000) >> 32;
        uint16_t t2 = ((codebook_entry) & 0x00000000FFFF0000) >> 16;
        uint16_t t3 = ((codebook_entry) & 0x000000000000FFFF) >> 0;

        uint32_t j = get_untwiddled_index(hw, h / 2, i);
        j = ((j % hw) * 2) + (((j / hw) * 2) * w);

        output[j] = t3;
        output[j + 1] = t1;
        output[j + w] = t2;
        output[j + w + 1] = t0;
    }
}

#define TWIDTAB(x) ( (x&1)|((x&2)<<1)|((x&4)<<2)|((x&8)<<3)|((x&16)<<4)| \
                     ((x&32)<<5)|((x&64)<<6)|((x&128)<<7)|((x&256)<<8)|((x&512)<<9) )
#define TWIDOUT(x, y) ( TWIDTAB((y)) | (TWIDTAB((x)) << 1) )

/* Convert RGBA 8888 (4 bytes per pixel) to ARGB 4444 (2 bytes per pixel) */
static void rgba8888_to_argb4444(const uint8_t* src, uint8_t* dst, uint32_t w, uint32_t h) {
    uint32_t count = w * h;
    const uint8_t* s = src;
    uint16_t* d = (uint16_t*) dst;

    for(uint32_t i = 0; i < count; ++i) {
        uint8_t r = s[0] >> 4;
        uint8_t g = s[1] >> 4;
        uint8_t b = s[2] >> 4;
        uint8_t a = s[3] >> 4;
        d[i] = (a << 12) | (r << 8) | (g << 4) | b;
        s += 4;
    }
}

/* Convert RGB 888 (3 bytes per pixel) to RGB 565 (2 bytes per pixel) */
static void rgb888_to_rgb565(const uint8_t* src, uint8_t* dst, uint32_t w, uint32_t h) {
    uint32_t count = w * h;
    const uint8_t* s = src;
    uint16_t* d = (uint16_t*) dst;

    for(uint32_t i = 0; i < count; ++i) {
        uint16_t r = s[0] >> 3;
        uint16_t g = s[1] >> 2;
        uint16_t b = s[2] >> 3;
        d[i] = (r << 11) | (g << 5) | b;
        s += 3;
    }
}

/* Convert single-channel 8-bit to RGB 565 (grayscale) */
static void r8_to_rgb565(const uint8_t* src, uint8_t* dst, uint32_t w, uint32_t h) {
    uint32_t count = w * h;
    const uint8_t* s = src;
    uint16_t* d = (uint16_t*) dst;

    for(uint32_t i = 0; i < count; ++i) {
        uint16_t v = *s++;
        uint16_t r = v >> 3;
        uint16_t g = v >> 2;
        uint16_t b = v >> 3;
        d[i] = (r << 11) | (g << 5) | b;
    }
}

static void untwiddle_16bpp(const uint8_t* src, uint8_t* dst, uint32_t w, uint32_t h) {
    uint32_t x, y, yout, min, mask;

    min = std::min(w, h);
    mask = min - 1;

    uint16_t* pixels;
    uint16_t* vtex;
    pixels = (uint16_t*) src;
    vtex = (uint16_t*) dst;

    for(y = 0; y < h; y++) {
        yout = y;
        for(x = 0; x < w; x++) {
            vtex[y * w + x] = pixels[
                TWIDOUT(x & mask, yout & mask) +
                (x / min + yout / min)*min * min
            ];
        }
    }
}

static TextureFormat uncompress_format(TextureFormat fmt) {
    switch(fmt) {
        case TEXTURE_FORMAT_ARGB_1US_1555_VQ_TWID: return TEXTURE_FORMAT_ARGB_1US_1555;
        case TEXTURE_FORMAT_ARGB_1US_4444_VQ_TWID: return TEXTURE_FORMAT_ARGB_1US_4444;
        case TEXTURE_FORMAT_RGB_1US_565_VQ_TWID: return TEXTURE_FORMAT_RGB_1US_565;
        case TEXTURE_FORMAT_ARGB_1US_1555_VQ_TWID_MIP: return TEXTURE_FORMAT_ARGB_1US_1555;
        case TEXTURE_FORMAT_ARGB_1US_4444_VQ_TWID_MIP: return TEXTURE_FORMAT_ARGB_1US_4444;
        case TEXTURE_FORMAT_RGB_1US_565_VQ_TWID_MIP: return TEXTURE_FORMAT_RGB_1US_565;
        default:
            return fmt;
    }
}

static TextureFormat rgba8888_format() { return TEXTURE_FORMAT_ARGB_1US_4444; }
static TextureFormat rgb888_format() { return TEXTURE_FORMAT_RGB_1US_565; }
static TextureFormat r8_format() { return TEXTURE_FORMAT_RGB_1US_565; }

static TextureFormat untwiddle_format(TextureFormat fmt) {
    switch(fmt) {
        case TEXTURE_FORMAT_ARGB_1US_1555_TWID: return TEXTURE_FORMAT_ARGB_1US_1555;
        case TEXTURE_FORMAT_ARGB_1US_4444_TWID: return TEXTURE_FORMAT_ARGB_1US_4444;
        case TEXTURE_FORMAT_RGB_1US_565_TWID: return TEXTURE_FORMAT_RGB_1US_565;
        case TEXTURE_FORMAT_ARGB_1US_1555_VQ_TWID_MIP: return TEXTURE_FORMAT_ARGB_1US_1555;
        case TEXTURE_FORMAT_ARGB_1US_4444_VQ_TWID_MIP: return TEXTURE_FORMAT_ARGB_1US_4444;
        case TEXTURE_FORMAT_RGB_1US_565_VQ_TWID_MIP: return TEXTURE_FORMAT_RGB_1US_565;
        default:
            return fmt;
    }
}


bool Renderer::convert_if_necessary(Texture* tex) {
    auto fmt = tex->format();
    if(natively_supports_texture_format(fmt)) {
        return true;
    }

    if(!supports_texture_format(fmt)) {
        return false;
    }

    const TextureFormat can_decompress [] = {
        TEXTURE_FORMAT_ARGB_1US_1555_VQ_TWID,
        TEXTURE_FORMAT_ARGB_1US_4444_VQ_TWID,
        TEXTURE_FORMAT_RGB_1US_565_VQ_TWID,
        TEXTURE_FORMAT_ARGB_1US_1555_VQ_TWID_MIP,
        TEXTURE_FORMAT_ARGB_1US_4444_VQ_TWID_MIP,
        TEXTURE_FORMAT_RGB_1US_565_VQ_TWID_MIP,
        (TextureFormat) 0
    };

    const TextureFormat can_untwiddle [] = {
        TEXTURE_FORMAT_ARGB_1US_1555_TWID,
        TEXTURE_FORMAT_ARGB_1US_4444_TWID,
        TEXTURE_FORMAT_RGB_1US_565_TWID,
        (TextureFormat) 0
    };

    bool decompress = format_in_list(fmt, can_decompress);

    if(decompress) {
        std::vector<uint8_t> tmp(tex->width() * tex->height() * 2);
        decompress_16bpp(&tex->data()[0], &tmp[0], tex->width(), tex->height());

        fmt = uncompress_format(fmt);
        tex->set_data(&tmp[0], tmp.size());
    }

    bool untwiddle = format_in_list(fmt, can_untwiddle);

    if(untwiddle) {
        std::vector<uint8_t> tmp(tex->data_size());

        untwiddle_16bpp(&tex->data()[0], &tmp[0], tex->width(), tex->height());
        fmt = untwiddle_format(fmt);
        tex->set_data(&tmp[0], tmp.size());
    }

    /* Handle 32-bit RGBA → 16-bit ARGB4444 conversion */
    if(fmt == TEXTURE_FORMAT_RGBA_4UB_8888) {
        std::vector<uint8_t> tmp(tex->width() * tex->height() * 2);
        rgba8888_to_argb4444(&tex->data()[0], &tmp[0], tex->width(), tex->height());
        fmt = rgba8888_format();
        tex->set_data(&tmp[0], tmp.size());
    }

    /* Handle 24-bit RGB → 16-bit RGB565 conversion */
    if(fmt == TEXTURE_FORMAT_RGB_3UB_888) {
        std::vector<uint8_t> tmp(tex->width() * tex->height() * 2);
        rgb888_to_rgb565(&tex->data()[0], &tmp[0], tex->width(), tex->height());
        fmt = rgb888_format();
        tex->set_data(&tmp[0], tmp.size());
    }

    /* Handle 8-bit grayscale → 16-bit RGB565 conversion */
    if(fmt == TEXTURE_FORMAT_R_1UB_8) {
        std::vector<uint8_t> tmp(tex->width() * tex->height() * 2);
        r8_to_rgb565(&tex->data()[0], &tmp[0], tex->width(), tex->height());
        fmt = r8_format();
        tex->set_data(&tmp[0], tmp.size());
    }

    /* Handle paletted formats - expand to actual color data then convert to RGB565 or ARGB4444 */
    if(tex->is_paletted_format()) {
        /* Paletted textures store the palette first, then index data.
         * The texture class maintains this structure. We need to expand
         * the palette indices to actual colors. */
        auto pal_size = tex->palette_size();
        auto data_size = tex->data_size();
        auto w = tex->width();
        auto h = tex->height();

        /* Determine output format based on whether the palette has alpha */
        /* For simplicity, we convert everything to ARGB4444 to preserve alpha if present */
        std::vector<uint8_t> tmp(w * h * 2);
        uint16_t* out = (uint16_t*)&tmp[0];

        /* Get palette data - for paletted formats the data buffer starts with the palette */
        const uint8_t* palette = &tex->data()[0];
        const uint8_t* index_data = &tex->data()[pal_size];

        /* Determine bits per index */
        bool is_4bit = (fmt == TEXTURE_FORMAT_RGB8_PALETTED4 ||
                        fmt == TEXTURE_FORMAT_RGBA8_PALETTED4 ||
                        fmt == TEXTURE_FORMAT_RGB565_PALETTED4);

        uint32_t pixel_count = w * h;
        for(uint32_t i = 0; i < pixel_count; ++i) {
            uint8_t idx;
            if(is_4bit) {
                /* 4 bits per pixel - 2 pixels per byte */
                uint8_t byte = index_data[i / 2];
                if(i & 1) {
                    idx = byte & 0xF;  /* Low nibble */
                } else {
                    idx = (byte >> 4) & 0xF;  /* High nibble */
                }
            } else {
                /* 8 bits per pixel */
                idx = index_data[i];
            }

            /* Read palette entry */
            uint8_t r, g, b, a = 255;
            switch(fmt) {
                case TEXTURE_FORMAT_RGB8_PALETTED4:
                case TEXTURE_FORMAT_RGB8_PALETTED8:
                    r = palette[idx * 3 + 0];
                    g = palette[idx * 3 + 1];
                    b = palette[idx * 3 + 2];
                    break;
                case TEXTURE_FORMAT_RGBA8_PALETTED4:
                case TEXTURE_FORMAT_RGBA8_PALETTED8:
                    r = palette[idx * 4 + 0];
                    g = palette[idx * 4 + 1];
                    b = palette[idx * 4 + 2];
                    a = palette[idx * 4 + 3];
                    break;
                case TEXTURE_FORMAT_RGB565_PALETTED4:
                case TEXTURE_FORMAT_RGB565_PALETTED8: {
                    uint16_t rgb565 = ((uint16_t)palette[idx * 2 + 0]) |
                                     (((uint16_t)palette[idx * 2 + 1]) << 8);
                    r = ((rgb565 >> 11) & 0x1F) << 3;
                    g = ((rgb565 >> 5) & 0x3F) << 2;
                    b = (rgb565 & 0x1F) << 3;
                    break;
                }
                default:
                    r = g = b = 255;
                    break;
            }

            /* Convert to ARGB4444 */
            out[i] = ((a >> 4) << 12) | ((r >> 4) << 8) | ((g >> 4) << 4) | (b >> 4);
        }

        fmt = TEXTURE_FORMAT_ARGB_1US_4444;
        tex->set_data(&tmp[0], tmp.size());
    }

    tex->set_format(fmt);

    /* Shouldn't happen, but just in case something goes wrong */
    assert(natively_supports_texture_format(tex->format()));
    if(!natively_supports_texture_format(tex->format())) {
        return false;
    }

    return true;
}

void Renderer::prepare_texture(Texture* tex) {
    assert(tex);

    if(!convert_if_necessary(tex)) {
        S_ERROR("Unable to convert texture to supported format");
        return;
    }

    on_texture_prepare(tex);
}

void Renderer::prepare_material(Material* material) {
    assert(material);
    on_material_prepare(material);
}


}
