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

TextureLoadResult DTEXLoader::do_load(std::shared_ptr<FileIfstream> stream) {
    TextureLoadResult result;

    DTexHeader header;

    stream->seekg(0);
    stream->read((char*) &header, sizeof(DTexHeader));

    bool untwiddled = (header.type & (1 << 26));
    bool twiddled = !untwiddled;
    bool compressed = (header.type & (1 << 30)) > 0;
    bool mipmapped = (header.type & (1 << 31)) > 0;
    bool strided = (header.type & (1 << 25)) > 0;
    uint32_t format = (header.type >> 27) & 0b111;

    if(strided || format > 2) {
        S_ERROR(
            "Strided, paletted or bumpmap .dtex textures are currently unsupported. Type: {0}, Format: {1}",
             header.type, format
        );

        /* FIXME: Failure?!? */
        return result;
    }

    uint32_t COMPRESSED_MASK = 4;
    uint32_t TWIDDLED_MASK = 2;
    uint32_t MIPMAPPED_MASK = 1;

    TextureFormat lookup[8] = {TEXTURE_FORMAT_INVALID};

    switch(format) {
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
            return result; /* FIXME: Failure... */
    }

    result.width = header.width;
    result.height = header.height;
    result.format = lookup[(compressed << 2) | (twiddled << 1) | mipmapped];

    if(result.format == TEXTURE_FORMAT_INVALID) {
        S_ERROR("Unsupported .dtex texture format");
        return result; //FIXME: Failure
    }

    result.data.resize(header.size);

    /* Read the data \o/ */
    stream->read((char*) &result.data[0], header.size);

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

        std::size_t base_size = ((header.width / 2) * (header.height / 2));
        std::size_t offset = calc_vq_mipmap_size(header.width / 2);
        result.data.insert(result.data.begin() + 2048, result.data.begin() + 2048 + offset, result.data.end());
        result.data.resize(base_size + 2048);
        result.data.shrink_to_fit();
        if(result.format == TEXTURE_FORMAT_ARGB_1US_4444_VQ_TWID_MIP) {
            result.format = TEXTURE_FORMAT_ARGB_1US_4444_VQ_TWID;
        } else if(result.format == TEXTURE_FORMAT_ARGB_1US_1555_VQ_TWID_MIP) {
            result.format = TEXTURE_FORMAT_ARGB_1US_1555_VQ_TWID;
        } else if(result.format == TEXTURE_FORMAT_RGB_1US_565_VQ_TWID_MIP) {
            result.format = TEXTURE_FORMAT_RGB_1US_565_VQ_TWID;
        } else {
            S_ERROR("Unexpected compressed + mipmapped format: {0}", result.format);
        }
    }
#endif

    auto renderer = get_app()->window->renderer.get();
    if(!renderer->supports_texture_format(result.format)) {
        return result; // FAILURE!
    }

    result.channels = texture_format_channels(result.format);
    return result;
}

}
}
