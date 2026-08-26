#include "dtex_writer.h"

#include <cstdio>
#include <vector>

#include <simulant/logging.h>

namespace sprite_gen {

namespace {

/* Bit layout for the .dtex 'type' field, as produced by texconv - see
 * simulant/loaders/dtex_loader.cpp and texconv's common.h. */
constexpr uint32_t PIXELFORMAT_ARGB4444 = 2;
constexpr uint32_t PIXELFORMAT_SHIFT = 27;
constexpr uint32_t FLAG_NONTWIDDLED = 1u << 26;

} // namespace

bool write_dtex(const std::string& path, uint16_t width, uint16_t height,
                const uint8_t* rgba) {
    FILE* fp = fopen(path.c_str(), "wb");
    if(!fp) {
        S_ERROR("Could not open '{0}' for writing", path);
        return false;
    }

    const std::size_t data_size = std::size_t(width) * height * 2;
    // Texture data must be a multiple of 32 bytes for DMA transfers - both
    // sheet dimensions are already rounded to powers of two, so this is
    // already satisfied, but pad defensively to match the format's contract.
    const std::size_t padded_size = (data_size + 31) & ~std::size_t(31);

    const uint32_t type = (PIXELFORMAT_ARGB4444 << PIXELFORMAT_SHIFT) |
                          FLAG_NONTWIDDLED;

    uint8_t header[16] = {
        'D', 'T', 'E', 'X',
        uint8_t(width & 0xFF),        uint8_t((width >> 8) & 0xFF),
        uint8_t(height & 0xFF),       uint8_t((height >> 8) & 0xFF),
        uint8_t(type & 0xFF),         uint8_t((type >> 8) & 0xFF),
        uint8_t((type >> 16) & 0xFF), uint8_t((type >> 24) & 0xFF),
        uint8_t(padded_size & 0xFF),         uint8_t((padded_size >> 8) & 0xFF),
        uint8_t((padded_size >> 16) & 0xFF), uint8_t((padded_size >> 24) & 0xFF),
    };

    if(fwrite(header, sizeof(header), 1, fp) != 1) {
        S_ERROR("Failed to write DTEX header to '{0}'", path);
        fclose(fp);
        return false;
    }

    // ARGB4444, packed little-endian: low byte = (G<<4)|B, high byte =
    // (A<<4)|R - matches texconv's to16BPP() output byte order.
    std::vector<uint8_t> row(std::size_t(width) * 2);
    for(uint16_t y = 0; y < height; ++y) {
        const uint8_t* src = rgba + (std::size_t(y) * width * 4);
        for(uint16_t x = 0; x < width; ++x) {
            uint8_t r = src[x * 4 + 0] >> 4;
            uint8_t g = src[x * 4 + 1] >> 4;
            uint8_t b = src[x * 4 + 2] >> 4;
            uint8_t a = src[x * 4 + 3] >> 4;
            row[x * 2 + 0] = uint8_t((g << 4) | b);
            row[x * 2 + 1] = uint8_t((a << 4) | r);
        }

        if(fwrite(row.data(), row.size(), 1, fp) != 1) {
            S_ERROR("Failed to write DTEX pixel data to '{0}'", path);
            fclose(fp);
            return false;
        }
    }

    const std::size_t padding = padded_size - data_size;
    if(padding > 0) {
        std::vector<uint8_t> zeros(padding, 0);
        if(fwrite(zeros.data(), zeros.size(), 1, fp) != 1) {
            S_ERROR("Failed to write DTEX padding to '{0}'", path);
            fclose(fp);
            return false;
        }
    }

    fclose(fp);
    return true;
}

} // namespace sprite_gen
