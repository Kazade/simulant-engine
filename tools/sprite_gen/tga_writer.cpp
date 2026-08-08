#include "tga_writer.h"

#include <cstdio>
#include <vector>

#include <simulant/logging.h>

namespace sprite_gen {

bool write_tga(const std::string& path, uint16_t width, uint16_t height,
               const uint8_t* rgba) {
    FILE* fp = fopen(path.c_str(), "wb");
    if(!fp) {
        S_ERROR("Could not open '{0}' for writing", path);
        return false;
    }

    /* 18 byte uncompressed TGA header. Image type 2 = uncompressed
     * true-color. Descriptor byte 0x28 = 8 bits of alpha (bits 0-3) plus
     * the top-left origin flag (bit 5), so rows are written top-to-bottom
     * matching the row order the sprite sheet is composed in - no flip
     * needed here. */
    uint8_t header[18] = {};
    header[2] = 2;
    header[12] = (uint8_t) (width & 0xFF);
    header[13] = (uint8_t) ((width >> 8) & 0xFF);
    header[14] = (uint8_t) (height & 0xFF);
    header[15] = (uint8_t) ((height >> 8) & 0xFF);
    header[16] = 32;
    header[17] = 0x28;

    if(fwrite(header, sizeof(header), 1, fp) != 1) {
        S_ERROR("Failed to write TGA header to '{0}'", path);
        fclose(fp);
        return false;
    }

    /* TGA pixel data is BGRA, our buffer is RGBA - swizzle row by row. */
    std::vector<uint8_t> row(std::size_t(width) * 4);
    for(uint16_t y = 0; y < height; ++y) {
        const uint8_t* src = rgba + (std::size_t(y) * width * 4);
        for(uint16_t x = 0; x < width; ++x) {
            row[x * 4 + 0] = src[x * 4 + 2]; // B
            row[x * 4 + 1] = src[x * 4 + 1]; // G
            row[x * 4 + 2] = src[x * 4 + 0]; // R
            row[x * 4 + 3] = src[x * 4 + 3]; // A
        }

        if(fwrite(row.data(), row.size(), 1, fp) != 1) {
            S_ERROR("Failed to write TGA pixel data to '{0}'", path);
            fclose(fp);
            return false;
        }
    }

    fclose(fp);
    return true;
}

} // namespace sprite_gen
