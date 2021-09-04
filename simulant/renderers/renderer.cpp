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

void Renderer::register_texture(TextureID tex_id, Texture* texture) {
    on_texture_register(tex_id, texture);
    texture_registry_.insert(std::make_pair(tex_id, texture));
}

void Renderer::unregister_texture(TextureID texture_id, Texture* texture) {
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
        case TEXTURE_FORMAT_ARGB_1US_4444_TWID:
        case TEXTURE_FORMAT_ARGB_1US_4444_VQ_TWID:
        case TEXTURE_FORMAT_RGB_1US_565_TWID:
        case TEXTURE_FORMAT_RGB_1US_565_VQ_TWID:
            return true;
        default:
            return false;
    }
}

bool Renderer::is_texture_registered(TextureID texture_id) const {
    return texture_registry_.count(texture_id);
}

void Renderer::pre_render() {
    for(auto wptr: texture_registry_){
        prepare_texture(wptr.second);
    }
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
        default:
            return fmt;
    }
}

static TextureFormat untwiddle_format(TextureFormat fmt) {
    switch(fmt) {
        case TEXTURE_FORMAT_ARGB_1US_1555_TWID: return TEXTURE_FORMAT_ARGB_1US_1555;
        case TEXTURE_FORMAT_ARGB_1US_4444_TWID: return TEXTURE_FORMAT_ARGB_1US_4444;
        case TEXTURE_FORMAT_RGB_1US_565_TWID: return TEXTURE_FORMAT_RGB_1US_565;
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
