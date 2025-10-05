#include <cstdint>

#include "../application.h"
#include "../renderers/renderer.h"
#include "../window.h"

#include "dtex_loader.h"

namespace smlt {
namespace loaders {

struct DTexHeader {
    uint8_t	 id[4];	// 'DTEX'
    uint16_t width;
    uint16_t height;
    uint32_t type;
    uint32_t size;
};

bool DTEXLoaderType::supports(const Path &filename) const {
    /* FIXME: supports() should be passed the first X bytes of the file
     * so we can check the DTEX id */
    return filename.ext() == ".dtex";
}

bool DTEXLoader::do_load(std::shared_ptr<FileIfstream> stream,
                         Texture* result) {
    DTexHeader header;

    stream->seekg(0);
    stream->read((char*) &header, sizeof(DTexHeader));

    bool untwiddled = (header.type & (1 << 26));
    bool twiddled = !untwiddled;
    bool compressed = (header.type & (1 << 30)) > 0;
    bool mipmapped = (header.type & (1 << 31)) > 0;
    bool strided = (header.type & (1 << 25)) > 0;
    uint32_t dformat = (header.type >> 27) & 0b111;

    if(strided || dformat > 2) {
        S_ERROR("Strided, paletted or bumpmap .dtex textures are currently "
                "unsupported. Type: {0}, Format: {1}",
                header.type, dformat);

        return false;
    }

    uint32_t COMPRESSED_MASK = 4;
    uint32_t TWIDDLED_MASK = 2;
    uint32_t MIPMAPPED_MASK = 1;

    TextureFormat lookup[8] = {TEXTURE_FORMAT_INVALID};

    switch(dformat) {
        case 0:
            lookup[COMPRESSED_MASK] = TEXTURE_FORMAT_ARGB_1US_1555_VQ_TWID;
            lookup[COMPRESSED_MASK | TWIDDLED_MASK] = TEXTURE_FORMAT_ARGB_1US_1555_VQ_TWID;
            lookup[COMPRESSED_MASK | MIPMAPPED_MASK] = TEXTURE_FORMAT_ARGB_1US_1555_VQ_TWID_MIP;
            lookup[COMPRESSED_MASK | TWIDDLED_MASK | MIPMAPPED_MASK] = TEXTURE_FORMAT_ARGB_1US_1555_VQ_TWID_MIP;
            lookup[TWIDDLED_MASK] = TEXTURE_FORMAT_ARGB_1US_1555_TWID;
            lookup[TWIDDLED_MASK | MIPMAPPED_MASK] = TEXTURE_FORMAT_ARGB_1US_1555_TWID;
            lookup[0] = TEXTURE_FORMAT_ARGB_1US_1555;
        break;
        case 1:
            lookup[COMPRESSED_MASK] = TEXTURE_FORMAT_RGB_1US_565_VQ_TWID;
            lookup[COMPRESSED_MASK | TWIDDLED_MASK] = TEXTURE_FORMAT_RGB_1US_565_VQ_TWID;
            lookup[COMPRESSED_MASK | MIPMAPPED_MASK] = TEXTURE_FORMAT_RGB_1US_565_VQ_TWID_MIP;
            lookup[COMPRESSED_MASK | TWIDDLED_MASK | MIPMAPPED_MASK] = TEXTURE_FORMAT_RGB_1US_565_VQ_TWID_MIP;
            lookup[TWIDDLED_MASK] = TEXTURE_FORMAT_RGB_1US_565_TWID;
            lookup[TWIDDLED_MASK | MIPMAPPED_MASK] = TEXTURE_FORMAT_RGB_1US_565_TWID;
            lookup[0] = TEXTURE_FORMAT_RGB_1US_565;
        break;
        case 2:
            lookup[COMPRESSED_MASK] = TEXTURE_FORMAT_ARGB_1US_4444_VQ_TWID;
            lookup[COMPRESSED_MASK | TWIDDLED_MASK] = TEXTURE_FORMAT_ARGB_1US_4444_VQ_TWID;
            lookup[COMPRESSED_MASK | MIPMAPPED_MASK] = TEXTURE_FORMAT_ARGB_1US_4444_VQ_TWID_MIP;
            lookup[COMPRESSED_MASK | TWIDDLED_MASK | MIPMAPPED_MASK] = TEXTURE_FORMAT_ARGB_1US_4444_VQ_TWID_MIP;
            lookup[TWIDDLED_MASK] = TEXTURE_FORMAT_ARGB_1US_4444_TWID;
            lookup[TWIDDLED_MASK | MIPMAPPED_MASK] = TEXTURE_FORMAT_ARGB_1US_4444_TWID;
            lookup[0] = TEXTURE_FORMAT_ARGB_1US_4444;
        break;
        default:
            S_ERROR("Unknown .dtex format");
            return false;
    }

    auto format =
        lookup[(TextureFormat)(compressed << 2) |
               (TextureFormat)(twiddled << 1) | (TextureFormat)mipmapped];

    if(format == TEXTURE_FORMAT_INVALID) {
        S_ERROR("Unsupported .dtex texture format");
        return false; // FIXME: Failure
    }

    uint8_t* data = nullptr;

#ifndef __DREAMCAST__
    if(compressed && mipmapped) {
        /* Non DC platforms can't deal with the mipmap data trailing
         * the main data. So we simply drop the mipmaps and pretend they
         * don't exist. However mipmaps are *first* in the data, so we skip them */
        auto calc_vq_mipmap_size = [](std::size_t s) -> std::size_t {
            std::size_t ret = 0;
            while(s != 1) {
                ret += (s / 2) * (s / 2);
                s /= 2;
            }

            ret += (1 * 1);
            return ret;
        };

        if(format == TEXTURE_FORMAT_ARGB_1US_4444_VQ_TWID_MIP) {
            format = TEXTURE_FORMAT_ARGB_1US_4444_VQ_TWID;
        } else if(format == TEXTURE_FORMAT_ARGB_1US_1555_VQ_TWID_MIP) {
            format = TEXTURE_FORMAT_ARGB_1US_1555_VQ_TWID;
        } else if(format == TEXTURE_FORMAT_RGB_1US_565_VQ_TWID_MIP) {
            format = TEXTURE_FORMAT_RGB_1US_565_VQ_TWID;
        } else {
            S_ERROR("Unexpected compressed + mipmapped format: {0}", format);
            return false;
        }

        std::size_t offset = calc_vq_mipmap_size(header.width / 2);
        auto final_data_size = header.size - offset;
        result->set_format(format);
        result->resize(header.width, header.height);

        data = result->map_data(final_data_size);

        // Read the VQ codebook
        stream->read((char*)data, 2048);

        // Seek past the mipmaps
        stream->seekg(offset);

        // Read the rest of the data
        stream->read((char*)data + 2048, header.size - 2048 - offset);
    }
#else
    result->set_format(format);
    result->resize(header.width, header.height);

    data = result->map_data(header.size);

    stream->read((char*)data, header.size);
#endif

    auto renderer = get_app()->window->renderer.get();
    if(!renderer->supports_texture_format(format)) {
        S_ERROR("Renderer doesn't support the texture format");
        return false;
    }

    return true;
}
}
}
