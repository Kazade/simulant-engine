#include <cstdint>

#include "../application.h"
#include "../renderers/renderer.h"
#include "../window.h"

#include "dtex_loader.h"

namespace smlt {
namespace loaders {

struct DTexHeader {
    uint8_t	 id[4];	// 'DTEX'
    int16_t width;
    int16_t height;
    int32_t type;
    int32_t	size;
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

    bool twiddled = (header.type & (1 << 26)) < 1;
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

    /* FIXME: Currently mipmaps are not loaded. In the case of VQ compressed mipmaps
     * the mipmap data follows the level 0 data and so these will require a new
     * texture format as mipmaps are not loaded in the traditional way (level-by-level) */

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

    result.data.resize(header.size);
    result.width = header.width;
    result.height = header.height;
    result.format = lookup[(compressed << 2) | (twiddled << 1) | mipmapped];

    if(result.format == TEXTURE_FORMAT_INVALID) {
        S_ERROR("Unsupported .dtex texture format");
        return result; //FIXME: Failure
    }

    /* Read the data \o/ */
    stream->read((char*) &result.data[0], header.size);

    auto renderer = get_app()->window->renderer.get();
    if(!renderer->supports_texture_format(result.format)) {
        return result; // FAILURE!
    }

    result.channels = texture_format_channels(result.format);
    return result;
}

}
}
