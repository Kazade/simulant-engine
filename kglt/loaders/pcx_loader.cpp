#include "pcx_loader.h"

namespace kglt {
namespace loaders {

#pragma pack(push)
#pragma pack(1)

/* PCX header */
struct Header {
    uint8_t manufacturer;
    uint8_t version;
    uint8_t encoding;
    uint8_t bits_per_pixel;

    uint16_t xmin, ymin;
    uint16_t xmax, ymax;
    uint16_t horz_res, vert_res;

    uint8_t palette[48];
    uint8_t reserved;
    uint8_t num_color_planes;

    uint16_t bytes_per_scan_line;
    uint16_t palette_type;
    uint16_t horz_size, vert_size;

    uint8_t padding[54];
};

#pragma pack(pop)

TextureLoadResult PCXLoader::do_load(const std::vector<uint8_t> &buffer) {
    TextureLoadResult result;

    Header* header = (Header*)&buffer[0];

    if(header->manufacturer != 0x0a) {
        throw std::runtime_error("Unsupported PCX manufacturer");
    }

    result.width = header->xmax - header->xmin + 1;
    result.height = header->ymax - header->ymin + 1;
    result.channels = 4;
    result.data.resize(result.width * result.height * result.channels);

    auto bitcount = header->bits_per_pixel * header->num_color_planes;

    uint8_t palette_marker = buffer[buffer.size() - 769];

    const uint8_t* palette = (palette_marker == 0x0c) ? &buffer[buffer.size() - 768] : header->palette;

    int32_t rle_count = 0;
    int32_t rle_value = 0;

    uint8_t cursor = 128;
    uint8_t* texel = &result.data[0];

    for(uint32_t idx = 0; idx < (result.width * result.height); ++idx) {
        if(rle_count == 0) {
            rle_value = buffer[cursor];
            if(rle_value > 0xbf) {
                rle_count = 0x3f & rle_value;
                ++cursor;
                rle_value = buffer[cursor];
            } else {
                rle_count = 1;
            }
        }

        rle_count--;

        texel[(idx * result.channels) + 0] = palette[(rle_value * 3) + 0];
        texel[(idx * result.channels) + 1] = palette[(rle_value * 3) + 1];
        texel[(idx * result.channels) + 2] = palette[(rle_value * 3) + 2];
        texel[(idx * result.channels) + 3] = 255;

        cursor++;
    }

    return result;
}

}
}
