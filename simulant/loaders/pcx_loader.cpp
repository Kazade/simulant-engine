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

#include "pcx_loader.h"

namespace smlt {
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

TextureLoadResult PCXLoader::do_load(std::shared_ptr<FileIfstream> stream) {
    TextureLoadResult result;

    stream->seekg(0, std::ios::end);
    int size = stream->tellg();
    stream->seekg(0, std::ios::beg);

    Header header;
    stream->read((char*) &header, sizeof(Header));

    if(header.manufacturer != 0x0a) {
        throw std::runtime_error("Unsupported PCX manufacturer");
    }

    result.width = header.xmax - header.xmin + 1;
    result.height = header.ymax - header.ymin + 1;
    result.channels = 3;
    result.format = TEXTURE_FORMAT_RGB_3UB_888;
    result.data.resize(result.width * result.height * result.channels);

    auto bitcount = header.bits_per_pixel * header.num_color_planes;
    if(bitcount != 8) {
        throw std::runtime_error("Unsupported PCX bitcount");
    }

    stream->seekg(size - 769);

    uint8_t palette_marker;
    stream->read((char*) &palette_marker, sizeof(uint8_t));

    const uint8_t* palette;

    if(palette_marker == 12) {
        palette = new uint8_t[768];
        stream->seekg(size - 768);
        stream->read((char*) &palette[0], sizeof(uint8_t) * 768);
    } else {
        palette = header.palette;
    }

    int32_t rle_count = 0;
    int32_t rle_value = 0;

    stream->seekg(128);

    uint8_t image_data;
    stream->read((char*) &image_data, sizeof(uint8_t));

    for(uint32_t idx = 0; idx < (result.width * result.height * result.channels); idx += result.channels) {

        if(rle_count == 0) {
            rle_value = image_data;
            stream->read((char*) &image_data, sizeof(uint8_t));

            if((rle_value & 0xc0) == 0xc0) {
                rle_count = rle_value & 0x3f;
                rle_value = image_data;
                stream->read((char*) &image_data, sizeof(uint8_t));
            } else {
                rle_count = 1;
            }
        }

        rle_count--;

        assert(rle_value * 3 < 768);

        result.data[idx + 0] = palette[(rle_value * 3) + 0];
        result.data[idx + 1] = palette[(rle_value * 3) + 1];
        result.data[idx + 2] = palette[(rle_value * 3) + 2];
    }

    if(palette != header.palette) {
        delete [] palette;
    }

    return result;
}

}
}
