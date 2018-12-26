#include "tex_texture_loader.h"
#include "loader.h"

namespace smlt {
namespace loaders {

struct TexHeader{
    char        id[4];	// 'DTEX'
    uint16_t	width;
    uint16_t	height;
    uint32_t	type;
    uint32_t	size;
};


enum PixelFormat {
    ARGB1555,
    RGB565,
    ARGB4444,
    YUV422,
    BUMPMAP,
    PAL4BPP,
    PAL8BPP,
};


TextureLoadResult TexTextureLoader::do_load(const std::vector<uint8_t> &buffer) {
    TextureLoadResult result;

    TexHeader* header = (TexHeader*) &buffer[0];

    bool twiddled = (header->type & (1 << 26)) < 1;
    bool compressed = (header->type & (1 << 30)) > 0;
    bool mipmapped = (header->type & (1 << 31)) > 0;
    bool strided = (header->type & (1 << 25)) > 0;
    uint32_t format = (header->type >> 27) & 0b111;

    /* FIXME: Support all formats, currently just do the untwiddled, unstrided formats without mipmaps */

    if(twiddled || strided) {
        L_ERROR("Unsupported .tex image format");
        return result; // Fail
    }

    TextureFormat format = TEXTURE_FORMAT_RGBA4444;
    uint8_t channels = 4;
    switch(format) {
    case RGB565:
        format = TEXTURE_FORMAT_RGB565;
        channels = 3;
    break;
    default: {
        L_ERROR(_F("Unsupported .tex format {0}").format(format));
        return result;
    }
    }

    result.width = header->width;
    result.height = header->height;
    result.channels = channels;

    uint8_t* data = &buffer[sizeof(TexHeader)];
    result.data.assign(data, data + header->size);

    return result;
}


}
}
